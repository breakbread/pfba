//
// Created by cpasjuste on 01/12/16.
//

#ifdef __NX__

#include <burner.h>
#include <gui/config.h>
#include <switch.h>
#include "video_nx.h"

using namespace c2d;

static unsigned int myHighCol16(int r, int g, int b, int /* i */) {
    unsigned int t;
    t = (unsigned int) ((r << 8) & 0xf800); // rrrr r000 0000 0000
    t |= (g << 3) & 0x07e0; // 0000 0ggg ggg0 0000
    t |= (b >> 3) & 0x001f; // 0000 0000 000b bbbb
    return t;
}

void NXVideo::clear() {

    u32 x, y, w, h;
    u32 *buf;

    for (int i = 0; i < 2; i++) {
        buf = (u32 *) gfxGetFramebuffer(&w, &h);
        for (y = 0; y < h; y++) {
            for (x = 0; x < w; x++) {
                buf[(u32) gfxGetFramebufferDisplayOffset((u32) x, (u32) y)]
                        = 0xFF000000;
            }
        }
        gfxFlushBuffers();
        gfxSwapBuffers();
        gfxWaitForVsync();
    }
}

NXVideo::NXVideo(Gui *gui, const c2d::Vector2f &size) : Texture(size, C2D_TEXTURE_FMT_RGB565) {

    this->gui = gui;

    printf("game resolution: %ix%i\n", (int) getSize().x, (int) getSize().y);

    if (BurnDrvGetFlags() & BDF_ORIENTATION_VERTICAL) {
        printf("game orientation: vertical\n");
    }
    if (BurnDrvGetFlags() & BDF_ORIENTATION_FLIPPED) {
        printf("game orientation: flipped\n");
    }

    nBurnBpp = 2;
    BurnHighCol = myHighCol16;
    BurnRecalcPal();

    pixels = (unsigned char *) malloc((size_t) (size.x * size.y * bpp));

    updateScaling();

    clear();
}

int NXVideo::lock(c2d::FloatRect *rect, void **pix, int *p) {

    if (!rect) {
        *pix = pixels;
    } else {
        *pix = (void *) (pixels + (int) rect->top * pitch + (int) rect->left * bpp);
    }

    if (p) {
        *p = pitch;
    }

    return 0;
}

void NXVideo::unlock() {

    if (scaling > 2) {
        // this can scale to any w/h, but slow for now
        double sw = getScale().x;
        double sh = getScale().y;
        int dst_w = (int) (getSize().x * sw);
        int dst_h = (int) (getSize().y * sh);
        int src_w = (int) getSize().x;
        int cx = (1280 - dst_w) / 2;
        int cy = (720 - dst_h) / 2;

        u32 *dst = (u32 *) gfxGetFramebuffer(NULL, NULL);
        unsigned short *src = (unsigned short *) pixels;
        unsigned int p, r, g, b;

        for (int y = 0; y < dst_h; y++) {
            for (int x = 0; x < dst_w; x++) {

                p = src[((int) (y / sh) * src_w) + (int) (x / sw)];
                r = ((p & 0xf800) >> 11);
                g = ((p & 0x07e0) >> 5);
                b = (p & 0x001f);

                dst[(u32) gfxGetFramebufferDisplayOffset((u32) x + cx, (u32) y + cy)] =
                        RGBA8_MAXALPHA(r << 3, g << 2, b << 3);
            }
        }
    } else {
        // fast 2x and 3x scaling, from retroarch
        int tgtw, tgth, centerx, centery;
        unsigned short *q;
        unsigned int v, r, g, b;
        unsigned subx, suby;
        int x, y, w, h, xsf, ysf, sf;

        w = (int) getSize().x;
        h = (int) getSize().y;
        xsf = sf = (int) getScale().x;
        ysf = (int) getScale().y;
        if (ysf < sf) {
            sf = ysf;
        }
        tgtw = w * sf;
        tgth = h * sf;
        centerx = (1280 - tgtw) / 2;
        centery = (720 - tgth) / 2;

        q = (unsigned short *) pixels;
        u32 *buffer = (u32 *) gfxGetFramebuffer(NULL, NULL);

        for (x = 0; x < w; x++) {
            for (y = 0; y < h; y++) {

                v = q[y * w + x];
                r = (v & 0xf800) >> 11;
                g = (v & 0x07e0) >> 5;
                b = v & 0x001f;

                u32 pixel = RGBA8_MAXALPHA(r << 3, g << 2, b << 3);
                for (subx = 0; subx < xsf; subx++) {
                    for (suby = 0; suby < ysf; suby++) {
                        buffer[(u32) gfxGetFramebufferDisplayOffset(
                                (u32) ((x * sf) + subx + centerx),
                                (u32) ((y * sf) + suby + centery))] = pixel;
                    }
                }
            }
        }
    }
}

void NXVideo::updateScaling() {

    int rotation = 0;
    scaling = gui->getConfig()->getValue(Option::Index::ROM_SCALING, true);

    // TODO: force right to left orientation on psp2,
    // should add platform specific code
    if ((gui->getConfig()->getValue(Option::Index::ROM_ROTATION, true) == 0
         || gui->getConfig()->getValue(Option::Index::ROM_ROTATION, true) == 3)
        && BurnDrvGetFlags() & BDF_ORIENTATION_VERTICAL) {
        if (!(BurnDrvGetFlags() & BDF_ORIENTATION_FLIPPED)) {
            rotation = 180;
        }
    } else if (gui->getConfig()->getValue(Option::Index::ROM_ROTATION, true) == 2
               && BurnDrvGetFlags() & BDF_ORIENTATION_VERTICAL) {
        if ((BurnDrvGetFlags() & BDF_ORIENTATION_FLIPPED)) {
            rotation = 180;
        }
    } else {
        if (BurnDrvGetFlags() & BDF_ORIENTATION_FLIPPED) {
            rotation = 90;
        } else if (BurnDrvGetFlags() & BDF_ORIENTATION_VERTICAL) {
            rotation = -90;
        } else {
            rotation = 0;
        }
    }

    c2d::Vector2f scale = getSize();

    switch (scaling) {

        case 1: // 2x
            scale.x = scale.x * 2;
            scale.y = scale.y * 2;
            break;

        case 2: // 3x
            scale.x = scale.x * 3;
            scale.y = scale.y * 3;
            break;

        case 3: // fit
            if (rotation == 0 || rotation == 180) {
                scale.y = gui->getRenderer()->getSize().y;
                scale.x = (int) (scale.x * (scale.y / getSize().y));
                if (scale.x > gui->getRenderer()->getSize().x) {
                    scale.x = gui->getRenderer()->getSize().x;
                    scale.y = (int) (scale.x * (getSize().y / getSize().x));
                }
            } else {
                scale.x = gui->getRenderer()->getSize().y;
                scale.y = (int) (scale.x * (getSize().y / getSize().x));
            }
            break;

        case 4: // fit 4:3
            if (rotation == 0 || rotation == 180) {
                scale.y = gui->getRenderer()->getSize().y;
                scale.x = (int) ((scale.y * 4.0) / 3.0);
                if (scale.x > gui->getRenderer()->getSize().x) {
                    scale.x = gui->getRenderer()->getSize().x;
                    scale.y = (int) ((scale.x * 3.0) / 4.0);
                }
            } else {
                scale.x = gui->getRenderer()->getSize().y;
                scale.y = (int) ((scale.x * 3.0) / 4.0);
            }
            break;

        case 5: // fullscreen
            if (rotation == 0 || rotation == 180) {
                scale.y = gui->getRenderer()->getSize().y;
                scale.x = gui->getRenderer()->getSize().x;
            } else {
                scale.y = gui->getRenderer()->getSize().x;
                scale.x = gui->getRenderer()->getSize().y;
            }
            break;

        default:
            break;
    }

    setOriginCenter();
    setPosition(gui->getRenderer()->getSize().x / 2, gui->getRenderer()->getSize().y / 2);
    setScale(scale.x / getSize().x, scale.y / getSize().y);
    setRotation(rotation);
}

NXVideo::~NXVideo() {

    // TODO: to free or not to free pBurnDraw?
    //pBurnDraw = NULL;
    if (pixels) {
        free(pixels);
    }
}

#endif