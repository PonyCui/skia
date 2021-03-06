/*
 * Copyright 2018 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "modules/skottie/src/SkottieAdapter.h"

#include "include/core/SkFont.h"
#include "include/core/SkMatrix.h"
#include "include/core/SkMatrix44.h"
#include "include/core/SkPath.h"
#include "include/core/SkRRect.h"
#include "include/private/SkTo.h"
#include "include/utils/Sk3D.h"
#include "modules/skottie/src/SkottieValue.h"
#include "modules/skottie/src/Transform.h"
#include "modules/sksg/include/SkSGDraw.h"
#include "modules/sksg/include/SkSGGradient.h"
#include "modules/sksg/include/SkSGGroup.h"
#include "modules/sksg/include/SkSGPaint.h"
#include "modules/sksg/include/SkSGPath.h"
#include "modules/sksg/include/SkSGRect.h"
#include "modules/sksg/include/SkSGTransform.h"
#include "modules/sksg/include/SkSGTrimEffect.h"

#include <cmath>
#include <utility>

namespace skottie {

RRectAdapter::RRectAdapter(sk_sp<sksg::RRect> wrapped_node)
    : fRRectNode(std::move(wrapped_node)) {}

RRectAdapter::~RRectAdapter() = default;

void RRectAdapter::apply() {
    // BM "position" == "center position"
    auto rr = SkRRect::MakeRectXY(SkRect::MakeXYWH(fPosition.x() - fSize.width() / 2,
                                                   fPosition.y() - fSize.height() / 2,
                                                   fSize.width(), fSize.height()),
                                  fRadius.width(),
                                  fRadius.height());
   fRRectNode->setRRect(rr);
}

RepeaterAdapter::RepeaterAdapter(sk_sp<sksg::RenderNode> repeater_node, Composite composite)
    : fRepeaterNode(repeater_node)
    , fComposite(composite)
    , fRoot(sksg::Group::Make()) {}

RepeaterAdapter::~RepeaterAdapter() = default;

void RepeaterAdapter::apply() {
    static constexpr SkScalar kMaxCount = 512;
    const auto count = static_cast<size_t>(SkTPin(fCount, 0.0f, kMaxCount) + 0.5f);

    const auto& compute_transform = [this] (size_t index) {
        const auto t = fOffset + index;

        // Position, scale & rotation are "scaled" by index/offset.
        SkMatrix m = SkMatrix::MakeTrans(-fAnchorPoint.x(),
                                         -fAnchorPoint.y());
        m.postScale(std::pow(fScale.x() * .01f, fOffset),
                    std::pow(fScale.y() * .01f, fOffset));
        m.postRotate(t * fRotation);
        m.postTranslate(t * fPosition.x() + fAnchorPoint.x(),
                        t * fPosition.y() + fAnchorPoint.y());

        return m;
    };

    // TODO: start/end opacity support.

    // TODO: we can avoid rebuilding all the fragments in most cases.
    fRoot->clear();
    for (size_t i = 0; i < count; ++i) {
        const auto insert_index = (fComposite == Composite::kAbove) ? i : count - i - 1;
        fRoot->addChild(sksg::TransformEffect::Make(fRepeaterNode,
                                                    compute_transform(insert_index)));
    }
}

PolyStarAdapter::PolyStarAdapter(sk_sp<sksg::Path> wrapped_node, Type t)
    : fPathNode(std::move(wrapped_node))
    , fType(t) {}

PolyStarAdapter::~PolyStarAdapter() = default;

void PolyStarAdapter::apply() {
    static constexpr int kMaxPointCount = 100000;
    const auto count = SkToUInt(SkTPin(SkScalarRoundToInt(fPointCount), 0, kMaxPointCount));
    const auto arc   = sk_ieee_float_divide(SK_ScalarPI * 2, count);

    const auto pt_on_circle = [](const SkPoint& c, SkScalar r, SkScalar a) {
        return SkPoint::Make(c.x() + r * std::cos(a),
                             c.y() + r * std::sin(a));
    };

    // TODO: inner/outer "roundness"?

    SkPath poly;

    auto angle = SkDegreesToRadians(fRotation - 90);
    poly.moveTo(pt_on_circle(fPosition, fOuterRadius, angle));
    poly.incReserve(fType == Type::kStar ? count * 2 : count);

    for (unsigned i = 0; i < count; ++i) {
        if (fType == Type::kStar) {
            poly.lineTo(pt_on_circle(fPosition, fInnerRadius, angle + arc * 0.5f));
        }
        angle += arc;
        poly.lineTo(pt_on_circle(fPosition, fOuterRadius, angle));
    }

    poly.close();
    fPathNode->setPath(poly);
}

GradientAdapter::GradientAdapter(sk_sp<sksg::Gradient> grad, size_t colorStopCount)
    : fGradient(std::move(grad))
    , fColorStopCount(colorStopCount) {}

template <typename T>
static inline const T* next_rec(const T* rec, const T* end_rec) {
    if (!rec) return nullptr;

    SkASSERT(rec < end_rec);
    rec++;

    return rec < end_rec ? rec : nullptr;
};

void GradientAdapter::apply() {
    this->onApply();

    // Gradient color stops are specified as a consolidated float vector holding:
    //
    //   a) an (optional) array of color/RGB stop records (t, r, g, b)
    //
    // followed by
    //
    //   b) an (optional) array of opacity/alpha stop records (t, a)
    //
    struct   ColorRec { float t, r, g, b; };
    struct OpacityRec { float t, a;       };

    // The number of color records is explicit (fColorStopCount),
    // while the number of opacity stops is implicit (based on the size of fStops).
    //
    // |fStops| holds ColorRec x |fColorStopCount| + OpacityRec x N
    const auto c_count = fColorStopCount,
               c_size  = c_count * 4,
               o_count = (fStops.size() - c_size) / 2;
    if (fStops.size() < c_size || fStops.size() != (c_count * 4 + o_count * 2)) {
        // apply() may get called before the stops are set, so only log when we have some stops.
        if (!fStops.empty()) {
            SkDebugf("!! Invalid gradient stop array size: %zu\n", fStops.size());
        }
        return;
    }

    const auto* c_rec = c_count > 0 ? reinterpret_cast<const ColorRec*>(fStops.data())
                                    : nullptr;
    const auto* o_rec = o_count > 0 ? reinterpret_cast<const OpacityRec*>(fStops.data() + c_size)
                                    : nullptr;
    const auto* c_end = c_rec + c_count;
    const auto* o_end = o_rec + o_count;

    sksg::Gradient::ColorStop current_stop = {
        0.0f, {
            c_rec ? c_rec->r : 0,
            c_rec ? c_rec->g : 0,
            c_rec ? c_rec->b : 0,
            o_rec ? o_rec->a : 1,
    }};

    std::vector<sksg::Gradient::ColorStop> stops;
    stops.reserve(c_count);

    // Merge-sort the color and opacity stops, LERP-ing intermediate channel values as needed.
    while (c_rec || o_rec) {
        // After exhausting one of color recs / opacity recs, continue propagating the last
        // computed values (as if they were specified at the current position).
        const auto& cs = c_rec
                ? *c_rec
                : ColorRec{ o_rec->t,
                            current_stop.fColor.fR,
                            current_stop.fColor.fG,
                            current_stop.fColor.fB };
        const auto& os = o_rec
                ? *o_rec
                : OpacityRec{ c_rec->t, current_stop.fColor.fA };

        // Compute component lerp coefficients based on the relative position of the stops
        // being considered. The idea is to select the smaller-pos stop, use its own properties
        // as specified (lerp with t == 1), and lerp (with t < 1) the properties from the
        // larger-pos stop against the previously computed gradient stop values.
        const auto     c_pos = std::max(cs.t, current_stop.fPosition),
                       o_pos = std::max(os.t, current_stop.fPosition),
                   c_pos_rel = c_pos - current_stop.fPosition,
                   o_pos_rel = o_pos - current_stop.fPosition,
                         t_c = SkTPin(sk_ieee_float_divide(o_pos_rel, c_pos_rel), 0.0f, 1.0f),
                         t_o = SkTPin(sk_ieee_float_divide(c_pos_rel, o_pos_rel), 0.0f, 1.0f);

        auto lerp = [](float a, float b, float t) { return a + t * (b - a); };

        current_stop = {
                std::min(c_pos, o_pos),
                {
                    lerp(current_stop.fColor.fR, cs.r, t_c ),
                    lerp(current_stop.fColor.fG, cs.g, t_c ),
                    lerp(current_stop.fColor.fB, cs.b, t_c ),
                    lerp(current_stop.fColor.fA, os.a, t_o)
                }
        };
        stops.push_back(current_stop);

        // Consume one of, or both (for coincident positions) color/opacity stops.
        if (c_pos <= o_pos) {
            c_rec = next_rec<ColorRec>(c_rec, c_end);
        }
        if (o_pos <= c_pos) {
            o_rec = next_rec<OpacityRec>(o_rec, o_end);
        }
    }

    stops.shrink_to_fit();
    fGradient->setColorStops(std::move(stops));
}

LinearGradientAdapter::LinearGradientAdapter(sk_sp<sksg::LinearGradient> grad, size_t stopCount)
    : INHERITED(std::move(grad), stopCount) {}

void LinearGradientAdapter::onApply() {
    auto* grad = static_cast<sksg::LinearGradient*>(fGradient.get());
    grad->setStartPoint(this->startPoint());
    grad->setEndPoint(this->endPoint());
}

RadialGradientAdapter::RadialGradientAdapter(sk_sp<sksg::RadialGradient> grad, size_t stopCount)
    : INHERITED(std::move(grad), stopCount) {}

void RadialGradientAdapter::onApply() {
    auto* grad = static_cast<sksg::RadialGradient*>(fGradient.get());
    grad->setStartCenter(this->startPoint());
    grad->setEndCenter(this->startPoint());
    grad->setStartRadius(0);
    grad->setEndRadius(SkPoint::Distance(this->startPoint(), this->endPoint()));
}

TrimEffectAdapter::TrimEffectAdapter(sk_sp<sksg::TrimEffect> trimEffect)
    : fTrimEffect(std::move(trimEffect)) {
    SkASSERT(fTrimEffect);
}

TrimEffectAdapter::~TrimEffectAdapter() = default;

void TrimEffectAdapter::apply() {
    // BM semantics: start/end are percentages, offset is "degrees" (?!).
    const auto  start = fStart  / 100,
                  end = fEnd    / 100,
               offset = fOffset / 360;

    auto startT = SkTMin(start, end) + offset,
          stopT = SkTMax(start, end) + offset;
    auto   mode = SkTrimPathEffect::Mode::kNormal;

    if (stopT - startT < 1) {
        startT -= SkScalarFloorToScalar(startT);
        stopT  -= SkScalarFloorToScalar(stopT);

        if (startT > stopT) {
            using std::swap;
            swap(startT, stopT);
            mode = SkTrimPathEffect::Mode::kInverted;
        }
    } else {
        startT = 0;
        stopT  = 1;
    }

    fTrimEffect->setStart(startT);
    fTrimEffect->setStop(stopT);
    fTrimEffect->setMode(mode);
}

} // namespace skottie
