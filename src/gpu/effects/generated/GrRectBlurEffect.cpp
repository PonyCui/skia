/*
 * Copyright 2018 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/**************************************************************************************************
 *** This file was autogenerated from GrRectBlurEffect.fp; do not modify.
 **************************************************************************************************/
#include "GrRectBlurEffect.h"

#include "include/gpu/GrTexture.h"
#include "src/gpu/glsl/GrGLSLFragmentProcessor.h"
#include "src/gpu/glsl/GrGLSLFragmentShaderBuilder.h"
#include "src/gpu/glsl/GrGLSLProgramBuilder.h"
#include "src/sksl/SkSLCPP.h"
#include "src/sksl/SkSLUtil.h"
class GrGLSLRectBlurEffect : public GrGLSLFragmentProcessor {
public:
    GrGLSLRectBlurEffect() {}
    void emitCode(EmitArgs& args) override {
        GrGLSLFPFragmentBuilder* fragBuilder = args.fFragBuilder;
        const GrRectBlurEffect& _outer = args.fFp.cast<GrRectBlurEffect>();
        (void)_outer;
        auto rect = _outer.rect;
        (void)rect;
        auto invSixSigma = _outer.invSixSigma;
        (void)invSixSigma;
        auto isFast = _outer.isFast;
        (void)isFast;
        highp = ((abs(rect.left()) > 16000.0 || abs(rect.top()) > 16000.0) ||
                 abs(rect.right()) > 16000.0) ||
                abs(rect.bottom()) > 16000.0;
        if (highp) {
            rectFVar = args.fUniformHandler->addUniform(kFragment_GrShaderFlag, kFloat4_GrSLType,
                                                        "rectF");
        }
        if (!highp) {
            rectHVar = args.fUniformHandler->addUniform(kFragment_GrShaderFlag, kHalf4_GrSLType,
                                                        "rectH");
        }
        invSixSigmaVar = args.fUniformHandler->addUniform(kFragment_GrShaderFlag, kHalf_GrSLType,
                                                          "invSixSigma");
        fragBuilder->codeAppendf(
                "/* key */ bool highp = %s;\nhalf xCoverage, yCoverage;\n@if (%s) {\n    half x, "
                "y;\n    @if (highp) {\n        x = max(half(%s.x - sk_FragCoord.x), "
                "half(sk_FragCoord.x - %s.z));\n        y = max(half(%s.y - sk_FragCoord.y), "
                "half(sk_FragCoord.y - %s.w));\n    } else {\n        x = max(half(float(%s.x) - "
                "sk_FragCoord.x), half(sk_FragCoord.x - float(%s.z)));\n        y = "
                "max(half(float(%s.y) - sk_FragCoord.y), half(sk_FragCoord.y - float(%s.w)));\n    "
                "}\n    xCoverage = sample(%s, float2(half2(x * %s, 0.5))).",
                (highp ? "true" : "false"), (_outer.isFast ? "true" : "false"),
                rectFVar.isValid() ? args.fUniformHandler->getUniformCStr(rectFVar) : "float4(0)",
                rectFVar.isValid() ? args.fUniformHandler->getUniformCStr(rectFVar) : "float4(0)",
                rectFVar.isValid() ? args.fUniformHandler->getUniformCStr(rectFVar) : "float4(0)",
                rectFVar.isValid() ? args.fUniformHandler->getUniformCStr(rectFVar) : "float4(0)",
                rectHVar.isValid() ? args.fUniformHandler->getUniformCStr(rectHVar) : "half4(0)",
                rectHVar.isValid() ? args.fUniformHandler->getUniformCStr(rectHVar) : "half4(0)",
                rectHVar.isValid() ? args.fUniformHandler->getUniformCStr(rectHVar) : "half4(0)",
                rectHVar.isValid() ? args.fUniformHandler->getUniformCStr(rectHVar) : "half4(0)",
                fragBuilder->getProgramBuilder()->samplerVariable(args.fTexSamplers[0]),
                args.fUniformHandler->getUniformCStr(invSixSigmaVar));
        fragBuilder->codeAppendf(
                "%s.w;\n    yCoverage = sample(%s, float2(half2(y * %s, 0.5))).%s.w;\n    %s = (%s "
                "* xCoverage) * yCoverage;\n} else {\n    half l, r, t, b;\n    @if (highp) {\n    "
                "    l = half(sk_FragCoord.x - %s.x);\n        r = half(%s.z - sk_FragCoord.x);\n  "
                "      t = half(sk_FragCoord.y - %s.y);\n        b = half(%s.w - "
                "sk_FragCoord.y);\n    } else {\n        l = half(sk_FragCoord.x - float(%s.x));\n "
                "       r = half(float(%s.z) - sk_FragCoord.x);\n        t = half(sk_FragCoord.y - "
                "float(%s.y));\n        b = half(float(",
                fragBuilder->getProgramBuilder()
                        ->samplerSwizzle(args.fTexSamplers[0])
                        .asString()
                        .c_str(),
                fragBuilder->getProgramBuilder()->samplerVariable(args.fTexSamplers[0]),
                args.fUniformHandler->getUniformCStr(invSixSigmaVar),
                fragBuilder->getProgramBuilder()
                        ->samplerSwizzle(args.fTexSamplers[0])
                        .asString()
                        .c_str(),
                args.fOutputColor, args.fInputColor,
                rectFVar.isValid() ? args.fUniformHandler->getUniformCStr(rectFVar) : "float4(0)",
                rectFVar.isValid() ? args.fUniformHandler->getUniformCStr(rectFVar) : "float4(0)",
                rectFVar.isValid() ? args.fUniformHandler->getUniformCStr(rectFVar) : "float4(0)",
                rectFVar.isValid() ? args.fUniformHandler->getUniformCStr(rectFVar) : "float4(0)",
                rectHVar.isValid() ? args.fUniformHandler->getUniformCStr(rectHVar) : "half4(0)",
                rectHVar.isValid() ? args.fUniformHandler->getUniformCStr(rectHVar) : "half4(0)",
                rectHVar.isValid() ? args.fUniformHandler->getUniformCStr(rectHVar) : "half4(0)");
        fragBuilder->codeAppendf(
                "%s.w) - sk_FragCoord.y);\n    }\n    half il = 1.0 + l * %s;\n    half ir = 1.0 + "
                "r * %s;\n    half it = 1.0 + t * %s;\n    half ib = 1.0 + b * %s;\n    xCoverage "
                "= (1.0 - sample(%s, float2(half2(il, 0.5))).%s.w) - sample(%s, float2(half2(ir, "
                "0.5))).%s.w;\n    yCoverage = (1.0 - sample(%s, float2(half2(it, 0.5))).%s.w) - "
                "sample(%s, float2(half2(ib, 0.5))).%s.w;\n}\n%s = (%s * xCoverage) * yCoverage;\n",
                rectHVar.isValid() ? args.fUniformHandler->getUniformCStr(rectHVar) : "half4(0)",
                args.fUniformHandler->getUniformCStr(invSixSigmaVar),
                args.fUniformHandler->getUniformCStr(invSixSigmaVar),
                args.fUniformHandler->getUniformCStr(invSixSigmaVar),
                args.fUniformHandler->getUniformCStr(invSixSigmaVar),
                fragBuilder->getProgramBuilder()->samplerVariable(args.fTexSamplers[0]),
                fragBuilder->getProgramBuilder()
                        ->samplerSwizzle(args.fTexSamplers[0])
                        .asString()
                        .c_str(),
                fragBuilder->getProgramBuilder()->samplerVariable(args.fTexSamplers[0]),
                fragBuilder->getProgramBuilder()
                        ->samplerSwizzle(args.fTexSamplers[0])
                        .asString()
                        .c_str(),
                fragBuilder->getProgramBuilder()->samplerVariable(args.fTexSamplers[0]),
                fragBuilder->getProgramBuilder()
                        ->samplerSwizzle(args.fTexSamplers[0])
                        .asString()
                        .c_str(),
                fragBuilder->getProgramBuilder()->samplerVariable(args.fTexSamplers[0]),
                fragBuilder->getProgramBuilder()
                        ->samplerSwizzle(args.fTexSamplers[0])
                        .asString()
                        .c_str(),
                args.fOutputColor, args.fInputColor);
    }

private:
    void onSetData(const GrGLSLProgramDataManager& pdman,
                   const GrFragmentProcessor& _proc) override {
        const GrRectBlurEffect& _outer = _proc.cast<GrRectBlurEffect>();
        { pdman.set1f(invSixSigmaVar, (_outer.invSixSigma)); }
        auto rect = _outer.rect;
        (void)rect;
        UniformHandle& rectF = rectFVar;
        (void)rectF;
        UniformHandle& rectH = rectHVar;
        (void)rectH;
        const GrSurfaceProxyView& integralView = _outer.textureSampler(0).view();
        GrTexture& integral = *integralView.proxy()->peekTexture();
        (void)integral;
        UniformHandle& invSixSigma = invSixSigmaVar;
        (void)invSixSigma;
        auto isFast = _outer.isFast;
        (void)isFast;

        float r[]{rect.fLeft, rect.fTop, rect.fRight, rect.fBottom};
        pdman.set4fv(highp ? rectF : rectH, 1, r);
    }
    bool highp = false;
    UniformHandle rectFVar;
    UniformHandle rectHVar;
    UniformHandle invSixSigmaVar;
};
GrGLSLFragmentProcessor* GrRectBlurEffect::onCreateGLSLInstance() const {
    return new GrGLSLRectBlurEffect();
}
void GrRectBlurEffect::onGetGLSLProcessorKey(const GrShaderCaps& caps,
                                             GrProcessorKeyBuilder* b) const {
    bool highp = ((abs(rect.left()) > 16000.0 || abs(rect.top()) > 16000.0) ||
                  abs(rect.right()) > 16000.0) ||
                 abs(rect.bottom()) > 16000.0;
    b->add32((int32_t)highp);
    b->add32((int32_t)isFast);
}
bool GrRectBlurEffect::onIsEqual(const GrFragmentProcessor& other) const {
    const GrRectBlurEffect& that = other.cast<GrRectBlurEffect>();
    (void)that;
    if (rect != that.rect) return false;
    if (integral != that.integral) return false;
    if (invSixSigma != that.invSixSigma) return false;
    if (isFast != that.isFast) return false;
    return true;
}
GrRectBlurEffect::GrRectBlurEffect(const GrRectBlurEffect& src)
        : INHERITED(kGrRectBlurEffect_ClassID, src.optimizationFlags())
        , rect(src.rect)
        , integral(src.integral)
        , invSixSigma(src.invSixSigma)
        , isFast(src.isFast) {
    this->setTextureSamplerCnt(1);
}
std::unique_ptr<GrFragmentProcessor> GrRectBlurEffect::clone() const {
    return std::unique_ptr<GrFragmentProcessor>(new GrRectBlurEffect(*this));
}
const GrFragmentProcessor::TextureSampler& GrRectBlurEffect::onTextureSampler(int index) const {
    return IthTextureSampler(index, integral);
}
GR_DEFINE_FRAGMENT_PROCESSOR_TEST(GrRectBlurEffect);
#if GR_TEST_UTILS
std::unique_ptr<GrFragmentProcessor> GrRectBlurEffect::TestCreate(GrProcessorTestData* data) {
    float sigma = data->fRandom->nextRangeF(3, 8);
    float width = data->fRandom->nextRangeF(200, 300);
    float height = data->fRandom->nextRangeF(200, 300);
    return GrRectBlurEffect::Make(data->proxyProvider(), *data->caps()->shaderCaps(),
                                  SkRect::MakeWH(width, height), sigma);
}
#endif
