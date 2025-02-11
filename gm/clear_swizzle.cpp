/*
 * Copyright 2020 Google LLC.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gm/gm.h"
#include "include/core/SkPoint.h"
#include "include/core/SkRect.h"
#include "include/private/SkColorData.h"
#include "src/core/SkCanvasPriv.h"
#include "src/gpu/GrSurfaceDrawContext.h"
#include "src/gpu/GrSwizzle.h"

namespace skiagm {

// Size of each clear
static constexpr int kSize = 64;

DEF_SIMPLE_GPU_GM_CAN_FAIL(clear_swizzle, rContext, canvas, errorMsg, 6*kSize, 2*kSize) {
    if (rContext->abandoned()) {
        *errorMsg = GM::kErrorMsg_DrawSkippedGpuOnly;
        return DrawResult::kSkip;
    }

    auto sdc = SkCanvasPriv::TopDeviceSurfaceDrawContext(canvas);
    if (!sdc) {
        *errorMsg = GM::kErrorMsg_DrawSkippedGpuOnly;
        return DrawResult::kSkip;
    }

    auto make_offscreen = [&](const SkISize dimensions) {
        GrSwizzle readSwizzle  = GrSwizzle::Concat(sdc->readSwizzle(), GrSwizzle{"bgra"});
        GrSwizzle writeSwizzle = GrSwizzle::Concat(sdc->readSwizzle(), GrSwizzle{"bgra"});
        return GrSurfaceFillContext::Make(rContext,
                                          kPremul_SkAlphaType,
                                          sdc->colorInfo().refColorSpace(),
                                          dimensions,
                                          SkBackingFit::kExact,
                                          sdc->asSurfaceProxy()->backendFormat(),
                                          /* sample count*/ 1,
                                          GrMipmapped::kNo,
                                          sdc->asSurfaceProxy()->isProtected(),
                                          readSwizzle,
                                          writeSwizzle,
                                          kTopLeft_GrSurfaceOrigin,
                                          SkBudgeted::kYes);
    };

    struct {
        SkIRect rect;
        SkPMColor4f color;
    } clears[] {
            {{    0,     0,   kSize,   kSize}, {1, 0, 0, 1}},
            {{kSize,     0, 2*kSize,   kSize}, {0, 1, 0, 1}},
            {{    0, kSize,   kSize, 2*kSize}, {0, 0, 1, 1}},
            {{kSize, kSize, 2*kSize, 2*kSize}, {1, 0, 1, 1}},
    };

    // onscreen for reference
    for (const auto& c : clears) {
        sdc->clear(c.rect, c.color);
    }

    // partial clear offscreen
    auto offscreen = make_offscreen({2*kSize, 2*kSize});
    for (const auto& c : clears) {
        offscreen->clear(c.rect, c.color);
    }
    sdc->blitTexture(offscreen->readSurfaceView(),
                     SkIRect::MakeSize({2*kSize, 2*kSize}),
                     SkIPoint{2*kSize, 0});

    // full offscreen clears
    for (const auto& c : clears) {
        offscreen = make_offscreen(c.rect.size());
        offscreen->clear(SkIRect::MakeSize(c.rect.size()), c.color);
        sdc->blitTexture(offscreen->readSurfaceView(),
                         SkIRect::MakeSize(offscreen->dimensions()),
                         c.rect.topLeft() + SkIPoint{4*kSize, 0});
    }

    return DrawResult::kOk;
}

} // namespace skiagm
