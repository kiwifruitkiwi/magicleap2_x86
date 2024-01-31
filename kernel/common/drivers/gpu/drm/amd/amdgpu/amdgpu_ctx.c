/*
 * Copyright (C) 2015-2022 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: monk liu <monk.liu@amd.com>
 */

#include <drm/drm_auth.h>
#include "amdgpu.h"
#include "amdgpu_sched.h"
#include "amdgpu_ras.h"

#include <linux/security.h>
#include <linux/sched.h>
#include <linux/string.h>

#define to_amdgpu_ctx_entity(e)	\
	container_of((e), struct amdgpu_ctx_entity, entity)

const unsigned int amdgpu_ctx_num_entities[AMDGPU_HW_IP_NUM] = {
	[AMDGPU_HW_IP_GFX]		=	2,
	[AMDGPU_HW_IP_COMPUTE]		=	8,
	[AMDGPU_HW_IP_DMA]		=	2,
	[AMDGPU_HW_IP_UVD]		=	1,
	[AMDGPU_HW_IP_VCE]		=	1,
	[AMDGPU_HW_IP_UVD_ENC]		=	1,
	[AMDGPU_HW_IP_VCN_DEC]		=	1,
	[AMDGPU_HW_IP_VCN_ENC]		=	1,
	[AMDGPU_HW_IP_VCN_JPEG]		=	1,
	[AMDGPU_HW_IP_VCN_JPEG_ENC]	=	1,
};

static int amdgpu_ctx_total_num_entities(void)
{
	unsigned i, num_entities = 0;

	for (i = 0; i < AMDGPU_HW_IP_NUM; ++i)
		num_entities += amdgpu_ctx_num_entities[i];

	return num_entities;
}

#define AMDGPU_PRIORITY_SCTX "u:r:ml_graphics:s0"

static bool is_ml_graphics_context(void)
{
	int rc;
	bool result = false;
	char *context = NULL;
	u32 len = 0;
	u32 secid = 0;

	/* get the security ID for the current task */
	security_task_getsecid(current, &secid);

	/* get the security context string from the ID */
	rc = security_secid_to_secctx(secid, &context, &len);
	if (rc)
		goto out;

	/* compare against the expected priority */
	result = context && strstarts(context, AMDGPU_PRIORITY_SCTX);

	if (context && !result)
		DRM_ERROR("unauthorized context: %s\n", context);

out:
	kfree(context);
	return result;
}

static int amdgpu_ctx_priority_permit(struct drm_file *filp,
				      enum drm_sched_priority priority)
{
	/* NORMAL and below are accessible by everyone */
	if (priority <= DRM_SCHED_PRIORITY_NORMAL)
		return 0;

	if (is_ml_graphics_context())
		return 0;

	if (drm_is_current_master(filp))
		return 0;

	return -EACCES;
}

static int amdgpu_ctx_init(struct amdgpu_device *adev,
			   enum drm_sched_priority priority,
			   struct drm_file *filp,
			   struct amdgpu_ctx *ctx)
{
	unsigned num_entities = amdgpu_ctx_total_num_entities();
	unsigned i, j;
	unsigned int sched_cnt, gfx_ring_cnt;
	int r;

	if (priority < 0 || priority >= DRM_SCHED_PRIORITY_MAX)
		return -EINVAL;

	r = amdgpu_ctx_priority_permit(filp, priority);
	if (r)
		return r;

	memset(ctx, 0, sizeof(*ctx));
	ctx->adev = adev;


	ctx->entities[0] = kcalloc(num_entities,
				   sizeof(struct amdgpu_ctx_entity),
				   GFP_KERNEL);
	if (!ctx->entities[0])
		return -ENOMEM;


	for (i = 0; i < num_entities; ++i) {
		struct amdgpu_ctx_entity *entity = &ctx->entities[0][i];

		entity->sequence = 1;
		entity->fences = kcalloc(amdgpu_sched_jobs,
					 sizeof(struct dma_fence*), GFP_KERNEL);
		if (!entity->fences) {
			r = -ENOMEM;
			goto error_cleanup_memory;
		}
		INIT_LIST_HEAD(&entity->sem_dep_list);
		mutex_init(&entity->sem_lock);
	}
	for (i = 1; i < AMDGPU_HW_IP_NUM; ++i)
		ctx->entities[i] = ctx->entities[i - 1] +
			amdgpu_ctx_num_entities[i - 1];

	kref_init(&ctx->refcount);
	spin_lock_init(&ctx->ring_lock);
	mutex_init(&ctx->lock);

	ctx->reset_counter = atomic_read(&adev->gpu_reset_counter);
	ctx->reset_counter_query = ctx->reset_counter;
	ctx->vram_lost_counter = atomic_read(&adev->vram_lost_counter);
	ctx->init_priority = priority;
	ctx->override_priority = DRM_SCHED_PRIORITY_UNSET;
	ctx->stable_pstate = AMDGPU_CTX_STABLE_PSTATE_NONE;

	for (i = 0; i < AMDGPU_HW_IP_NUM; ++i) {
		struct drm_gpu_scheduler **scheds = NULL;
		struct drm_gpu_scheduler *sched = NULL;
		unsigned num_scheds = 0;

		switch (i) {
		case AMDGPU_HW_IP_GFX:

			if (amdgpu_mcbp) {
				enum drm_sched_priority ctx_priority;
				u32 hqd_priority;

				ctx_priority = (ctx->override_priority == DRM_SCHED_PRIORITY_UNSET) ?
						ctx->init_priority : ctx->override_priority;

				hqd_priority = (ctx_priority <= DRM_SCHED_PRIORITY_NORMAL) ? 0 : 1;

				/* Populate the schedulers whose HW Queue's priority */
				/* can be mapped to the context priority */
				for (sched_cnt = 0, gfx_ring_cnt = 0;
				     gfx_ring_cnt < adev->gfx.num_gfx_rings;
				     gfx_ring_cnt++) {

					if (!(adev->gfx.gfx_ring[gfx_ring_cnt].ring_obj)
					    || (adev->gfx.gfx_ring[gfx_ring_cnt].hqd_queue_priority != hqd_priority))
						continue;
					ctx->gfx_sched[sched_cnt++] = &adev->gfx.gfx_ring[gfx_ring_cnt].sched;
				}

				ctx->num_gfx_sched = sched_cnt;
			}

			for (j = 0; j < amdgpu_ctx_num_entities[i]; j++) {
				if (!(adev->gfx.gfx_ring[j].ring_obj))
					continue;

				if (amdgpu_mcbp) {
					scheds = ctx->gfx_sched;
					num_scheds = ctx->num_gfx_sched;
				} else {
					sched = &adev->gfx.gfx_ring[j].sched;
					scheds = &sched;
					num_scheds = 1;
				}

				r = drm_sched_entity_init(&ctx->entities[i][j].entity,
							  priority, scheds,
							  num_scheds, &ctx->guilty);
				if (r)
					goto error_cleanup_entities;
			}
			break;
		case AMDGPU_HW_IP_COMPUTE:
			for (j = 0; j < amdgpu_ctx_num_entities[i]; j++) {
				if (!(adev->gfx.compute_ring[j].ring_obj))
					continue;

				/* Last compute ring is reserved for ACE
				 * tunneling. Make sure only one scheduler
				 * assigned, so that workload submitted
				 * to other rings not submitted to last ring
				 */
				if (adev->gfx.compute_ring[j].ace_tunnel_enabled) {
					sched = &adev->gfx.compute_ring[j].sched;
					scheds = &sched;
					num_scheds = 1;
				} else {
					scheds = adev->gfx.compute_sched;
					num_scheds = adev->gfx.num_compute_sched - 1;
				}
				r = drm_sched_entity_init(&ctx->entities[i][j].entity,
							  priority, scheds,
							  num_scheds, &ctx->guilty);
				if (r)
					goto error_cleanup_entities;
			}

			break;
		case AMDGPU_HW_IP_DMA:
			scheds = adev->sdma.sdma_sched;
			num_scheds = adev->sdma.num_sdma_sched;
			break;
		case AMDGPU_HW_IP_UVD:
			sched = &adev->uvd.inst[0].ring.sched;
			scheds = &sched;
			num_scheds = 1;
			break;
		case AMDGPU_HW_IP_VCE:
			sched = &adev->vce.ring[0].sched;
			scheds = &sched;
			num_scheds = 1;
			break;
		case AMDGPU_HW_IP_UVD_ENC:
			sched = &adev->uvd.inst[0].ring_enc[0].sched;
			scheds = &sched;
			num_scheds = 1;
			break;
		case AMDGPU_HW_IP_VCN_DEC:
			scheds = adev->vcn.vcn_dec_sched;
			num_scheds =  adev->vcn.num_vcn_dec_sched;
			break;
		case AMDGPU_HW_IP_VCN_ENC:
			scheds = adev->vcn.vcn_enc_sched;
			num_scheds =  adev->vcn.num_vcn_enc_sched;
			break;
		case AMDGPU_HW_IP_VCN_JPEG:
			scheds = adev->jpeg.jpeg_sched;
			num_scheds =  adev->jpeg.num_jpeg_sched;
			break;
		case AMDGPU_HW_IP_VCN_JPEG_ENC:
			scheds = adev->jpeg.jpeg_enc_sched;
			num_scheds =  adev->jpeg.num_jpeg_enc_sched;
			break;
		}

		/* Already initialized entity above for compute rings */
		if (i == AMDGPU_HW_IP_COMPUTE)
			continue;

		/* Already initialized entity above for Gfx rings */
		if (i == AMDGPU_HW_IP_GFX)
			continue;

		for (j = 0; j < amdgpu_ctx_num_entities[i]; ++j)

			r = drm_sched_entity_init(&ctx->entities[i][j].entity,
						  priority, scheds,
						  num_scheds, &ctx->guilty);
		if (r)
			goto error_cleanup_entities;
	}

	return 0;

error_cleanup_entities:
	for (i = 0; i < num_entities; ++i)
		drm_sched_entity_destroy(&ctx->entities[0][i].entity);

error_cleanup_memory:
	for (i = 0; i < num_entities; ++i) {
		struct amdgpu_ctx_entity *entity = &ctx->entities[0][i];

		kfree(entity->fences);
		entity->fences = NULL;
	}

	kfree(ctx->entities[0]);
	ctx->entities[0] = NULL;
	return r;
}

static int amdgpu_ctx_get_stable_pstate(struct amdgpu_ctx *ctx,
					u32 *stable_pstate)
{
	struct amdgpu_device *adev = NULL;
	enum amd_dpm_forced_level current_level;

	if (!ctx)
		return -EINVAL;

	adev = ctx->adev;
	if (is_support_sw_smu(adev))
		current_level = smu_get_performance_level(&adev->smu);
	else if (adev->powerplay.pp_funcs->get_performance_level)
		current_level = amdgpu_dpm_get_performance_level(adev);
	else
		current_level = adev->pm.dpm.forced_level;

	switch (current_level) {
	case AMD_DPM_FORCED_LEVEL_PROFILE_STANDARD:
		*stable_pstate = AMDGPU_CTX_STABLE_PSTATE_STANDARD;
		break;
	case AMD_DPM_FORCED_LEVEL_PROFILE_MIN_SCLK:
		*stable_pstate = AMDGPU_CTX_STABLE_PSTATE_MIN_SCLK;
		break;
	case AMD_DPM_FORCED_LEVEL_PROFILE_MIN_MCLK:
		*stable_pstate = AMDGPU_CTX_STABLE_PSTATE_MIN_MCLK;
		break;
	case AMD_DPM_FORCED_LEVEL_PROFILE_PEAK:
		*stable_pstate = AMDGPU_CTX_STABLE_PSTATE_PEAK;
		break;
	default:
		*stable_pstate = AMDGPU_CTX_STABLE_PSTATE_NONE;
		break;
	}
	return 0;
}

static int amdgpu_ctx_set_stable_pstate(struct amdgpu_ctx *ctx,
					u32 stable_pstate)
{
	struct amdgpu_device *adev = NULL;
	enum amd_dpm_forced_level level = AMD_DPM_FORCED_LEVEL_MANUAL;
	int r = 0;

	if (!ctx)
		return -EINVAL;

	adev = ctx->adev;
	mutex_lock(&adev->pm.stable_pstate_ctx_lock);
	if (adev->pm.stable_pstate_ctx && adev->pm.stable_pstate_ctx != ctx) {
		r = -EBUSY;
		goto done;
	}

	switch (stable_pstate) {
	case AMDGPU_CTX_STABLE_PSTATE_NONE:
		level = AMD_DPM_FORCED_LEVEL_MANUAL;
		break;
	case AMDGPU_CTX_STABLE_PSTATE_STANDARD:
		level = AMD_DPM_FORCED_LEVEL_PROFILE_STANDARD;
		break;
	case AMDGPU_CTX_STABLE_PSTATE_MIN_SCLK:
		level = AMD_DPM_FORCED_LEVEL_PROFILE_MIN_SCLK;
		break;
	case AMDGPU_CTX_STABLE_PSTATE_MIN_MCLK:
		level = AMD_DPM_FORCED_LEVEL_PROFILE_MIN_MCLK;
		break;
	case AMDGPU_CTX_STABLE_PSTATE_PEAK:
		level = AMD_DPM_FORCED_LEVEL_PROFILE_PEAK;
		break;
	default:
		r = -EINVAL;
		goto done;
	}

	if (is_support_sw_smu(adev))
		r = smu_force_performance_level(&adev->smu, level);
	else if (adev->powerplay.pp_funcs->force_performance_level)
		r = amdgpu_dpm_force_performance_level(adev, level);

	if (level == AMD_DPM_FORCED_LEVEL_MANUAL)
		adev->pm.stable_pstate_ctx = NULL;
	else
		adev->pm.stable_pstate_ctx = ctx;
done:
	mutex_unlock(&adev->pm.stable_pstate_ctx_lock);

	return r;
}

static void amdgpu_ctx_fini(struct kref *ref)
{
	struct amdgpu_ctx *ctx = container_of(ref, struct amdgpu_ctx, refcount);
	unsigned num_entities = amdgpu_ctx_total_num_entities();
	struct amdgpu_device *adev = ctx->adev;
	unsigned i, j;

	if (!adev)
		return;

	for (i = 0; i < num_entities; ++i) {
		struct amdgpu_ctx_entity *entity = &ctx->entities[0][i];

		for (j = 0; j < amdgpu_sched_jobs; ++j)
			dma_fence_put(entity->fences[j]);

		kfree(entity->fences);
	}

	kfree(ctx->entities[0]);
	amdgpu_ctx_set_stable_pstate(ctx, AMDGPU_CTX_STABLE_PSTATE_NONE);
	mutex_destroy(&ctx->lock);

	kfree(ctx);
}

int amdgpu_ctx_get_entity(struct amdgpu_ctx *ctx, u32 hw_ip, u32 instance,
			  u32 ring, struct drm_sched_entity **entity)
{
	struct amdgpu_device *adev = ctx->adev;

	if (hw_ip >= AMDGPU_HW_IP_NUM) {
		DRM_ERROR("unknown HW IP type: %d\n", hw_ip);
		return -EINVAL;
	}

	/* Right now all IPs have only one instance - multiple rings. */
	if (instance != 0) {
		DRM_DEBUG("invalid ip instance: %d\n", instance);
		return -EINVAL;
	}

	/* In case of MCBP for Gfx IP we ignore the RING ID passed and always  */
	/* operate on RING 0 so that GPU Schduler can choose the RING ID based */
	/* on priority and load on each scheduler */
	if (amdgpu_mcbp && (hw_ip == AMDGPU_HW_IP_GFX)) {
		ring = 0;
	}

	if (ring >= amdgpu_ctx_num_entities[hw_ip]) {
		DRM_DEBUG("invalid ring: %d %d\n", hw_ip, ring);
		return -EINVAL;
	}

	if (hw_ip == AMDGPU_HW_IP_COMPUTE) {
		if (ctx->init_priority == DRM_SCHED_PRIORITY_HIGH_HW) {
			/* For compute, all the HIGH_HW ctx, should use
			 * reserved ACE tunnel ring.
			 */
			ring = adev->gfx.num_compute_rings - 1;
		} else if (ring == adev->gfx.num_compute_rings - 1) {
			/* Only HIGH_HW ctx allowed to submit workload to ACE
			 * tunnel ring. Route IBs to Ring 0 if ctx is of lower
			 * priority.
			 */
			DRM_INFO("Low priority ctx tried using ACE ring\n");
			ring = 0;
		}
	}

	*entity = &ctx->entities[hw_ip][ring].entity;
	return 0;
}

static int amdgpu_ctx_alloc(struct amdgpu_device *adev,
			    struct amdgpu_fpriv *fpriv,
			    struct drm_file *filp,
			    enum drm_sched_priority priority,
			    uint32_t *id)
{
	struct amdgpu_ctx_mgr *mgr = &fpriv->ctx_mgr;
	struct amdgpu_ctx *ctx;
	int r;

	ctx = kmalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mutex_lock(&mgr->lock);
	r = idr_alloc(&mgr->ctx_handles, ctx, 1, AMDGPU_VM_MAX_NUM_CTX, GFP_KERNEL);
	if (r < 0) {
		mutex_unlock(&mgr->lock);
		kfree(ctx);
		return r;
	}

	*id = (uint32_t)r;
	r = amdgpu_ctx_init(adev, priority, filp, ctx);
	if (r) {
		idr_remove(&mgr->ctx_handles, *id);
		*id = 0;
		kfree(ctx);
	}
	mutex_unlock(&mgr->lock);
	return r;
}

static void amdgpu_ctx_do_release(struct kref *ref)
{
	struct amdgpu_ctx *ctx;
	unsigned num_entities;
	u32 i;

	ctx = container_of(ref, struct amdgpu_ctx, refcount);

	num_entities = amdgpu_ctx_total_num_entities();
	for (i = 0; i < num_entities; i++)
		drm_sched_entity_destroy(&ctx->entities[0][i].entity);

	amdgpu_ctx_fini(ref);
}

static int amdgpu_ctx_free(struct amdgpu_fpriv *fpriv, uint32_t id)
{
	struct amdgpu_ctx_mgr *mgr = &fpriv->ctx_mgr;
	struct amdgpu_ctx *ctx;

	mutex_lock(&mgr->lock);
	ctx = idr_remove(&mgr->ctx_handles, id);
	if (ctx)
		kref_put(&ctx->refcount, amdgpu_ctx_do_release);
	mutex_unlock(&mgr->lock);
	return ctx ? 0 : -EINVAL;
}

static int amdgpu_ctx_query(struct amdgpu_device *adev,
			    struct amdgpu_fpriv *fpriv, uint32_t id,
			    union drm_amdgpu_ctx_out *out)
{
	struct amdgpu_ctx *ctx;
	struct amdgpu_ctx_mgr *mgr;
	unsigned reset_counter;

	if (!fpriv)
		return -EINVAL;

	mgr = &fpriv->ctx_mgr;
	mutex_lock(&mgr->lock);
	ctx = idr_find(&mgr->ctx_handles, id);
	if (!ctx) {
		mutex_unlock(&mgr->lock);
		return -EINVAL;
	}

	/* TODO: these two are always zero */
	out->state.flags = 0x0;
	out->state.hangs = 0x0;

	/* determine if a GPU reset has occured since the last call */
	reset_counter = atomic_read(&adev->gpu_reset_counter);
	/* TODO: this should ideally return NO, GUILTY, or INNOCENT. */
	if (ctx->reset_counter_query == reset_counter)
		out->state.reset_status = AMDGPU_CTX_NO_RESET;
	else
		out->state.reset_status = AMDGPU_CTX_UNKNOWN_RESET;
	ctx->reset_counter_query = reset_counter;

	mutex_unlock(&mgr->lock);
	return 0;
}

static int amdgpu_ctx_query2(struct amdgpu_device *adev,
	struct amdgpu_fpriv *fpriv, uint32_t id,
	union drm_amdgpu_ctx_out *out)
{
	struct amdgpu_ctx *ctx;
	struct amdgpu_ctx_mgr *mgr;
	unsigned long ras_counter;

	if (!fpriv)
		return -EINVAL;

	mgr = &fpriv->ctx_mgr;
	mutex_lock(&mgr->lock);
	ctx = idr_find(&mgr->ctx_handles, id);
	if (!ctx) {
		mutex_unlock(&mgr->lock);
		return -EINVAL;
	}

	out->state.flags = 0x0;
	out->state.hangs = 0x0;

	if (ctx->reset_counter != atomic_read(&adev->gpu_reset_counter))
		out->state.flags |= AMDGPU_CTX_QUERY2_FLAGS_RESET;

	if (ctx->vram_lost_counter != atomic_read(&adev->vram_lost_counter))
		out->state.flags |= AMDGPU_CTX_QUERY2_FLAGS_VRAMLOST;

	if (atomic_read(&ctx->guilty))
		out->state.flags |= AMDGPU_CTX_QUERY2_FLAGS_GUILTY;

	/*query ue count*/
	ras_counter = amdgpu_ras_query_error_count(adev, false);
	/*ras counter is monotonic increasing*/
	if (ras_counter != ctx->ras_counter_ue) {
		out->state.flags |= AMDGPU_CTX_QUERY2_FLAGS_RAS_UE;
		ctx->ras_counter_ue = ras_counter;
	}

	/*query ce count*/
	ras_counter = amdgpu_ras_query_error_count(adev, true);
	if (ras_counter != ctx->ras_counter_ce) {
		out->state.flags |= AMDGPU_CTX_QUERY2_FLAGS_RAS_CE;
		ctx->ras_counter_ce = ras_counter;
	}

	mutex_unlock(&mgr->lock);
	return 0;
}

static int amdgpu_ctx_stable_pstate(struct amdgpu_device *adev,
				    struct amdgpu_fpriv *fpriv, uint32_t id,
				    bool set, u32 *stable_pstate)
{
	struct amdgpu_ctx *ctx;
	struct amdgpu_ctx_mgr *mgr;
	int r;

	if (!fpriv)
		return -EINVAL;

	mgr = &fpriv->ctx_mgr;
	mutex_lock(&mgr->lock);
	ctx = idr_find(&mgr->ctx_handles, id);
	if (!ctx) {
		mutex_unlock(&mgr->lock);
		return -EINVAL;
	}

	if (set)
		r = amdgpu_ctx_set_stable_pstate(ctx, *stable_pstate);
	else
		r = amdgpu_ctx_get_stable_pstate(ctx, stable_pstate);

	mutex_unlock(&mgr->lock);
	return r;
}

int amdgpu_ctx_ioctl(struct drm_device *dev, void *data,
		     struct drm_file *filp)
{
	int r;
	uint32_t id, stable_pstate;
	enum drm_sched_priority priority;

	union drm_amdgpu_ctx *args = data;
	struct amdgpu_device *adev = dev->dev_private;
	struct amdgpu_fpriv *fpriv = filp->driver_priv;

	r = 0;
	id = args->in.ctx_id;
	priority = amdgpu_to_sched_priority(args->in.priority);

	/* For backwards compatibility reasons, we need to accept
	 * ioctls with garbage in the priority field */
	if (priority == DRM_SCHED_PRIORITY_INVALID)
		priority = DRM_SCHED_PRIORITY_NORMAL;

	switch (args->in.op) {
	case AMDGPU_CTX_OP_ALLOC_CTX:
		r = amdgpu_ctx_alloc(adev, fpriv, filp, priority, &id);
		args->out.alloc.ctx_id = id;
		break;
	case AMDGPU_CTX_OP_FREE_CTX:
		r = amdgpu_ctx_free(fpriv, id);
		break;
	case AMDGPU_CTX_OP_QUERY_STATE:
		r = amdgpu_ctx_query(adev, fpriv, id, &args->out);
		break;
	case AMDGPU_CTX_OP_QUERY_STATE2:
		r = amdgpu_ctx_query2(adev, fpriv, id, &args->out);
		break;
	case AMDGPU_CTX_OP_GET_STABLE_PSTATE:
		if (args->in.flags)
			return -EINVAL;
		r = amdgpu_ctx_stable_pstate(adev, fpriv, id, false, &stable_pstate);
		args->out.pstate.flags = stable_pstate;
		break;
	case AMDGPU_CTX_OP_SET_STABLE_PSTATE:
		if (args->in.flags & ~AMDGPU_CTX_STABLE_PSTATE_FLAGS_MASK)
			return -EINVAL;
		stable_pstate = args->in.flags & AMDGPU_CTX_STABLE_PSTATE_FLAGS_MASK;
		if (stable_pstate > AMDGPU_CTX_STABLE_PSTATE_PEAK)
			return -EINVAL;
		r = amdgpu_ctx_stable_pstate(adev, fpriv, id, true, &stable_pstate);
		break;
	default:
		return -EINVAL;
	}

	return r;
}

struct amdgpu_ctx *amdgpu_ctx_get(struct amdgpu_fpriv *fpriv, uint32_t id)
{
	struct amdgpu_ctx *ctx;
	struct amdgpu_ctx_mgr *mgr;

	if (!fpriv)
		return NULL;

	mgr = &fpriv->ctx_mgr;

	mutex_lock(&mgr->lock);
	ctx = idr_find(&mgr->ctx_handles, id);
	if (ctx)
		kref_get(&ctx->refcount);
	mutex_unlock(&mgr->lock);
	return ctx;
}

int amdgpu_ctx_put(struct amdgpu_ctx *ctx)
{
	if (ctx == NULL)
		return -EINVAL;

	kref_put(&ctx->refcount, amdgpu_ctx_do_release);
	return 0;
}

void amdgpu_ctx_add_fence(struct amdgpu_ctx *ctx,
			  struct drm_sched_entity *entity,
			  struct dma_fence *fence, uint64_t* handle)
{
	struct amdgpu_ctx_entity *centity = to_amdgpu_ctx_entity(entity);
	uint64_t seq = centity->sequence;
	struct dma_fence *other = NULL;
	unsigned idx = 0;

	idx = seq & (amdgpu_sched_jobs - 1);
	other = centity->fences[idx];
	if (other)
		BUG_ON(!dma_fence_is_signaled(other));

	dma_fence_get(fence);

	spin_lock(&ctx->ring_lock);
	centity->fences[idx] = fence;
	centity->sequence++;
	spin_unlock(&ctx->ring_lock);

	dma_fence_put(other);
	if (handle)
		*handle = seq;
}

struct dma_fence *amdgpu_ctx_get_fence(struct amdgpu_ctx *ctx,
				       struct drm_sched_entity *entity,
				       uint64_t seq)
{
	struct amdgpu_ctx_entity *centity = to_amdgpu_ctx_entity(entity);
	struct dma_fence *fence;

	spin_lock(&ctx->ring_lock);

	if (seq == ~0ull)
		seq = centity->sequence - 1;

	if (seq >= centity->sequence) {
		spin_unlock(&ctx->ring_lock);
		return ERR_PTR(-EINVAL);
	}


	if (seq + amdgpu_sched_jobs < centity->sequence) {
		spin_unlock(&ctx->ring_lock);
		return NULL;
	}

	fence = dma_fence_get(centity->fences[seq & (amdgpu_sched_jobs - 1)]);
	spin_unlock(&ctx->ring_lock);

	return fence;
}

void amdgpu_ctx_priority_override(struct amdgpu_ctx *ctx,
				  enum drm_sched_priority priority)
{
	unsigned num_entities = amdgpu_ctx_total_num_entities();
	enum drm_sched_priority ctx_prio;
	unsigned i;

	ctx->override_priority = priority;

	ctx_prio = (ctx->override_priority == DRM_SCHED_PRIORITY_UNSET) ?
			ctx->init_priority : ctx->override_priority;

	for (i = 0; i < num_entities; i++) {
		struct drm_sched_entity *entity = &ctx->entities[0][i].entity;

		drm_sched_entity_set_priority(entity, ctx_prio);
	}
}

int amdgpu_ctx_wait_prev_fence(struct amdgpu_ctx *ctx,
			       struct drm_sched_entity *entity)
{
	struct amdgpu_ctx_entity *centity = to_amdgpu_ctx_entity(entity);
	struct dma_fence *other;
	unsigned idx;
	long r;

	spin_lock(&ctx->ring_lock);
	idx = centity->sequence & (amdgpu_sched_jobs - 1);
	other = dma_fence_get(centity->fences[idx]);
	spin_unlock(&ctx->ring_lock);

	if (!other)
		return 0;

	r = dma_fence_wait(other, true);
	if (r < 0 && r != -ERESTARTSYS)
		DRM_ERROR("Error (%ld) waiting for fence!\n", r);

	dma_fence_put(other);
	return r;
}

void amdgpu_ctx_mgr_init(struct amdgpu_ctx_mgr *mgr)
{
	mutex_init(&mgr->lock);
	idr_init(&mgr->ctx_handles);
}

long amdgpu_ctx_mgr_entity_flush(struct amdgpu_ctx_mgr *mgr, long timeout)
{
	unsigned num_entities = amdgpu_ctx_total_num_entities();
	struct amdgpu_ctx *ctx;
	struct idr *idp;
	uint32_t id, i;

	idp = &mgr->ctx_handles;

	mutex_lock(&mgr->lock);
	idr_for_each_entry(idp, ctx, id) {
		for (i = 0; i < num_entities; i++) {
			struct drm_sched_entity *entity;

			entity = &ctx->entities[0][i].entity;
			timeout = drm_sched_entity_flush(entity, timeout);
		}
	}
	mutex_unlock(&mgr->lock);
	return timeout;
}

void amdgpu_ctx_mgr_entity_fini(struct amdgpu_ctx_mgr *mgr)
{
	unsigned num_entities = amdgpu_ctx_total_num_entities();
	struct amdgpu_ctx *ctx;
	struct idr *idp;
	uint32_t id, i;

	idp = &mgr->ctx_handles;

	idr_for_each_entry(idp, ctx, id) {
		if (kref_read(&ctx->refcount) != 1) {
			DRM_ERROR("ctx %p is still alive\n", ctx);
			continue;
		}

		for (i = 0; i < num_entities; i++)
			drm_sched_entity_fini(&ctx->entities[0][i].entity);
	}
}

void amdgpu_ctx_mgr_fini(struct amdgpu_ctx_mgr *mgr)
{
	struct amdgpu_ctx *ctx;
	struct idr *idp;
	uint32_t id;

	amdgpu_ctx_mgr_entity_fini(mgr);

	idp = &mgr->ctx_handles;

	idr_for_each_entry(idp, ctx, id) {
		if (kref_put(&ctx->refcount, amdgpu_ctx_fini) != 1)
			DRM_ERROR("ctx %p is still alive\n", ctx);
	}

	idr_destroy(&mgr->ctx_handles);
	mutex_destroy(&mgr->lock);
}

void amdgpu_ctx_init_sched(struct amdgpu_device *adev)
{
	int i, j;

	for (i = 0; i < adev->gfx.num_gfx_rings; i++) {
		adev->gfx.gfx_sched[i] = &adev->gfx.gfx_ring[i].sched;
		adev->gfx.num_gfx_sched++;
	}

	for (i = 0; i < adev->gfx.num_compute_rings; i++) {
		adev->gfx.compute_sched[i] = &adev->gfx.compute_ring[i].sched;
		adev->gfx.num_compute_sched++;
	}

	for (i = 0; i < adev->sdma.num_instances; i++) {
		adev->sdma.sdma_sched[i] = &adev->sdma.instance[i].ring.sched;
		adev->sdma.num_sdma_sched++;
	}

	for (i = 0; i < adev->vcn.num_vcn_inst; ++i) {
		if (adev->vcn.harvest_config & (1 << i))
			continue;
		adev->vcn.vcn_dec_sched[adev->vcn.num_vcn_dec_sched++] =
			&adev->vcn.inst[i].ring_dec.sched;
	}

	for (i = 0; i < adev->vcn.num_vcn_inst; ++i) {
		if (adev->vcn.harvest_config & (1 << i))
			continue;
		for (j = 0; j < adev->vcn.num_enc_rings; ++j)
			adev->vcn.vcn_enc_sched[adev->vcn.num_vcn_enc_sched++] =
				&adev->vcn.inst[i].ring_enc[j].sched;
	}

	for (i = 0; i < adev->jpeg.num_jpeg_inst; ++i) {
		if (adev->jpeg.harvest_config & (1 << i))
			continue;
		adev->jpeg.jpeg_sched[adev->jpeg.num_jpeg_sched++] =
			&adev->jpeg.inst[i].ring_dec.sched;
		adev->jpeg.jpeg_enc_sched[adev->jpeg.num_jpeg_enc_sched++] =
			&adev->jpeg.inst[i].ring_enc.sched;
	}
}
