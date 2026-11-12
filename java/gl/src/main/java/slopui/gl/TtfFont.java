package slopui.gl;

import static org.lwjgl.stb.STBTruetype.*;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;
import org.lwjgl.stb.STBTTBakedChar;
import org.lwjgl.stb.STBTTFontinfo;
import org.lwjgl.system.MemoryStack;
import org.lwjgl.system.MemoryUtil;
import slopui.Font;
import slopui.Image;
import slopui.Painter;

/**
 * A TrueType font (stb_truetype): 'size' UI units per em, baked again for each device-pixel size it is drawn
 * at (sharp at any unit). Latin, Latin-1, Latin Extended A/B.
 */
public final class TtfFont implements Font {
    private static final int FIRST = 32, COUNT = 0x250 - 32;
    private final ByteBuffer data;
    private final STBTTFontinfo info = STBTTFontinfo.create();
    private final float size;
    private final float scaleEm;          // font units to UI units
    private final float ascent, lineH;
    private final Map<Integer, Float> advances = new HashMap<>();
    private float bakedUnit = -1;
    private Image atlas;
    private Painter owner;
    private STBTTBakedChar.Buffer baked;
    private int atlasW, atlasH;

    public TtfFont(InputStream in, float size) throws IOException {
        byte[] bytes = in.readAllBytes();
        data = MemoryUtil.memAlloc(bytes.length);
        data.put(bytes).flip();
        if (!stbtt_InitFont(info, data)) {
            throw new IOException("not a TrueType font");
        }
        this.size = size;
        scaleEm = stbtt_ScaleForMappingEmToPixels(info, size);
        try (MemoryStack s = MemoryStack.stackPush()) {
            var a = s.mallocInt(1);
            var d = s.mallocInt(1);
            var g = s.mallocInt(1);
            stbtt_GetFontVMetrics(info, a, d, g);
            ascent = a.get(0) * scaleEm;
            lineH = (a.get(0) - d.get(0)) * scaleEm;
        }
    }

    @Override
    public boolean has(int cp) {
        return cp == ' ' || cp >= FIRST && cp < FIRST + COUNT && stbtt_FindGlyphIndex(info, cp) != 0;
    }

    @Override
    public float advance(int cp) {
        Float a = advances.get(cp);
        if (a == null) {
            try (MemoryStack s = MemoryStack.stackPush()) {
                var adv = s.mallocInt(1);
                var lsb = s.mallocInt(1);
                stbtt_GetCodepointHMetrics(info, cp, adv, lsb);
                a = adv.get(0) * scaleEm;
            }
            advances.put(cp, a);
        }
        return a;
    }

    @Override
    public float lineHeight() {
        return lineH;
    }

    private void bake(Painter p) {
        float unit = p.unit();
        float px = stbtt_ScaleForMappingEmToPixels(info, size * unit) / stbtt_ScaleForPixelHeight(info, 1);
        int side = 256;
        ByteBuffer bitmap = null;
        STBTTBakedChar.Buffer chars = STBTTBakedChar.malloc(COUNT);
        while (side <= 4096) {
            bitmap = MemoryUtil.memAlloc(side * side);
            int r = stbtt_BakeFontBitmap(data, px, bitmap, side, side, FIRST, chars);
            if (r > 0) {
                break;
            }
            MemoryUtil.memFree(bitmap);
            bitmap = null;
            side *= 2;
        }
        if (bitmap == null) {
            chars.free();
            return;
        }
        int[] argb = new int[side * side];
        for (int i = 0; i < side * side; i++) {
            argb[i] = (bitmap.get(i) & 0xFF) << 24 | 0xFFFFFF;
        }
        MemoryUtil.memFree(bitmap);
        if (atlas != null) {
            atlas.dispose();
        }
        if (baked != null) {
            baked.free();
        }
        atlas = p.createImage(side, side, argb, false, false);
        atlasW = atlasH = side;
        baked = chars;
        bakedUnit = unit;
        owner = p;
    }

    @Override
    public void drawGlyph(Painter p, int cp, float x, float y, int argb) {
        if (bakedUnit != p.unit() || owner != p) {
            bake(p);
        }
        if (baked == null || cp < FIRST || cp >= FIRST + COUNT) {
            return;
        }
        STBTTBakedChar c = baked.get(cp - FIRST);
        float unit = p.unit();
        float bx = Math.round(x * unit), by = Math.round((y + ascent) * unit);
        float x0 = bx + c.xoff(), y0 = by + c.yoff();
        float gw = c.x1() - c.x0(), gh = c.y1() - c.y0();
        if (gw <= 0 || gh <= 0) {
            return;
        }
        slopui.Shapes.image(p, atlas, c.x0(), c.y0(), gw, gh, x0 / unit, y0 / unit, gw / unit, gh / unit, argb);
    }
}
