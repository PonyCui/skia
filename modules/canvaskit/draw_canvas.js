;function installDrawCanvas(CanvasKit) {

    var drawingPaint = new CanvasKit.SkPaint();
    var drawingCanvasMap = {};
    var drawingCanvasTexture = {};
    var drawingSkImageMap = {};

    if (CanvasKit.GetWebGLContext_Hooked === undefined) {
        var oriGetWebGLContext = CanvasKit.GetWebGLContext;
        CanvasKit.GetWebGLContext = function () {
            var canvas = arguments[0];
            if (typeof canvas === "string") {
                canvas = document.getElementById(canvas)
            }
            if (window.WebGL2RenderingContext !== undefined) {
                canvas.getContext("webgl");
            }
            var result = oriGetWebGLContext.apply(this, arguments);
            var gl = canvas.getContext("webgl");
            var isWebGL2 = gl != null && !(gl instanceof WebGLRenderingContext);
            if (!isWebGL2) {
                var oriBindTexture = gl.bindTexture;
                var currentTextureName;
                gl.bindTexture = function () {
                    currentTextureName = arguments[1] instanceof WebGLTexture ? arguments[1].name : undefined;
                    oriBindTexture.apply(this, arguments);
                }
                var oriDeleteTexture = gl.deleteTexture;
                gl.deleteTexture = function () {
                    var drawingIndex = drawingCanvasTexture[arguments[0].name];
                    if (drawingIndex !== undefined) {
                        drawingCanvasMap[drawingIndex].width = 0;
                        drawingCanvasMap[drawingIndex].height = 0;
                        delete drawingCanvasMap[drawingIndex];
                        delete drawingCanvasTexture[arguments[0].name];
                    }
                    oriDeleteTexture.apply(this, arguments);
                }
                var oriTexSubImage2D = gl.texSubImage2D
                gl.texSubImage2D = function () {
                    var buffer = arguments[8];
                    if (buffer instanceof Uint8Array && buffer[4] === 3 && buffer[5] === 2 && buffer[6] === 1 && buffer[7] === 0) {
                        var drawingIndex = convertColorToInt(buffer)
                        if (drawingCanvasMap[drawingIndex] !== undefined) {
                            var canvas = drawingCanvasMap[drawingIndex];
                            drawingCanvasTexture[currentTextureName] = drawingIndex;
                            gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, canvas);
                            oriTexSubImage2D.apply(this, [arguments[0], arguments[1], arguments[2], arguments[3], arguments[6], arguments[7], canvas]);
                            if (drawingSkImageMap[drawingIndex] !== undefined) {
                                drawingSkImageMap[drawingIndex].delete();
                                delete drawingSkImageMap[drawingIndex];
                            }
                        }
                        else {
                            oriTexSubImage2D.apply(this, arguments);
                        }
                    }
                    else {
                        oriTexSubImage2D.apply(this, arguments);
                    }
                };
            }
            else {
                throw "Not support WebGL2";
            }
            return result;
        }
        CanvasKit.GetWebGLContext_Hooked = true;
    }
    if (CanvasKit.SkCanvas.prototype.drawCanvas === undefined) {
        CanvasKit.SkCanvas.prototype.drawCanvas = function (canvas, dstRect, paint) {
            var drawingIndex = canvas._drawingIndex !== undefined ? canvas._drawingIndex : genCanvasId();
            drawingCanvasMap[drawingIndex] = canvas;
            canvas._drawingIndex = drawingIndex;
            var data = convertIntToColor(drawingIndex);
            data.push(3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0)
            var cachedImage = CanvasKit.MakeImage(data, 2, 2, CanvasKit.AlphaType.Unpremul, CanvasKit.ColorType.RGBA_8888);
            drawingSkImageMap[drawingIndex] = cachedImage;
            cachedImage.canvasWidth = canvas.width;
            cachedImage.canvasHeight = canvas.height;
            this.drawImageRect(
                cachedImage,
                { fLeft: 0, fTop: 0, fRight: 2, fBottom: 2 },
                dstRect || { fLeft: 0, fTop: 0, fRight: canvas.width, fBottom: canvas.height },
                paint || drawingPaint,
                true
            );
            return drawingSkImageMap[drawingIndex];
        }
    }
    if (CanvasKit.SkCanvas.prototype.makeCanvasImage === undefined) {
        CanvasKit.SkCanvas.prototype.makeCanvasImage = function (canvas, dstRect, paint) {
            var pictureRecorder = new CanvasKit.SkPictureRecorder();
            var targetRect = dstRect || { fLeft: 0, fTop: 0, fRight: canvas.width, fBottom: canvas.height };
            var pictureCanvas = pictureRecorder.beginRecording({
                fLeft: 0,
                fTop: 0,
                fRight: targetRect.fRight,
                fBottom: targetRect.fBottom,
            })
            pictureCanvas.drawCanvas(canvas, targetRect, paint || drawingPaint);
            pictureCanvas.flush()
            var picture = pictureRecorder.finishRecordingAsPicture()
            pictureRecorder.delete()
            return CanvasKit.MakeImageFromPicture(picture, {
                width: targetRect.fRight,
                height: targetRect.fBottom,
                alphaType: CanvasKit.AlphaType.Unpremul,
                colorType: CanvasKit.ColorType.RGBA_8888,
            });
        }
    }

    function convertIntToColor(value) {
        return [((value >> 16) & 0xff), ((value >> 8) & 0xff), ((value) & 0xff), ((value >> 24) & 0xff)];
    }

    function convertColorToInt(values) {
        return CanvasKit.Color(values[0], values[1], values[2], values[3]);
    }


    var _nextCanvasId = 0
    var _maxCanvasId = Math.pow(2, 24) - 1;
    function genCanvasId() {
        if (_nextCanvasId > _maxCanvasId) _nextCanvasId = 0;
        _nextCanvasId++;
        return _nextCanvasId;
    }

}

if (window.CanvasKitInit !== undefined) {
    var oriCanvasKitInit = window.CanvasKitInit;
    window.CanvasKitInit = function () {
        var canvasKitInitResult = oriCanvasKitInit.apply(this, arguments);
        var oriReady = canvasKitInitResult.ready;
        canvasKitInitResult.ready = function () {
            var readyResult = oriReady.apply(this, arguments);
            return readyResult.then(function (it) {
                installDrawCanvas(it);
                return it;
            })
        }
        return canvasKitInitResult
    }
};