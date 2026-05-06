#include "Renderer2D.h"
#include "Math/Math.h"

static std::unordered_map<char, BLRgba32> sColorMap = {
    {'0', BLRgba32(255, 255, 255)},
    {'1', BLRgba32(173, 244, 255)},
    {'2', BLRgba32(73, 252, 0)},
    {'3', BLRgba32(191, 218, 255)},
    {'4', BLRgba32(255, 39, 29)},
    {'5', BLRgba32(235, 183, 255)},
    {'6', BLRgba32(255, 202, 111)},
    {'7', BLRgba32(230, 230, 230)},
    {'8', BLRgba32(255, 148, 69)},
    {'9', BLRgba32(255, 238, 125)},
    {'!', BLRgba32(209, 255, 249)},
    {'@', BLRgba32(255, 205, 201)},
    {'#', BLRgba32(255, 143, 243)},
    {'$', BLRgba32(255, 252, 197)},
    {'^', BLRgba32(181, 255, 151)},
    {'&', BLRgba32(254, 235, 255)},
    {'w', BLRgba32(255, 255, 255)},
    {'o', BLRgba32(252, 230, 186)},
    {'b', BLRgba32(0, 0, 0)},
    {'p', BLRgba32(255, 223, 241)},
    {'q', BLRgba32(12, 96, 164)},
    {'e', BLRgba32(25, 185, 255)},
    {'r', BLRgba32(111, 211, 87)},
    {'t', BLRgba32(47, 131, 13)},
    {'a', BLRgba32(81, 81, 81)},
    {'s', BLRgba32(158, 158, 158)},
    {'c', BLRgba32(80, 255, 255)}
};

Renderer2D::Renderer2D()
: m_threadCount(0)
{
}

Renderer2D::~Renderer2D()
{
}

void Renderer2D::Init(int32 width, int32 height)
{
    m_surface = BLImage(width, height, BL_FORMAT_PRGB32);

    BLContextCreateInfo createInfo{};
    if(m_threadCount > 0) {
        createInfo.thread_count = m_threadCount;
    }

    m_context = BLContext(m_surface, createInfo);
    m_context.clear_all();
}

void Renderer2D::DrawRect(const BLRect& rect, const BLRgba32& color)
{
    m_context.fill_rect(rect, color);
}

void Renderer2D::DrawImage(BLImage* pImage, const BLRect& drawRect, const BLRgba32& color, float rotateAngle)
{
    if(!pImage) {
        return;
    }

    if(color.a() == 0) {
        return;
    }

    m_context.set_comp_op(BL_COMP_OP_SRC_OVER);

    if(rotateAngle != 0.0f) {
        m_context.save();

        float cX = drawRect.x + drawRect.w * 0.5;
        float cY = drawRect.y + drawRect.h * 0.5;
        
        m_context.translate(cX, cY);
        m_context.rotate(rotateAngle * M_PI / 180.0f);
        m_context.translate(-cX, -cY);
    }

    m_context.blit_image(drawRect, *pImage);

    if(color != BLRgba32(255,255,255,255)) {
        
    }

    if(rotateAngle != 0.0f) {
        m_context.restore();
    }
}

void Renderer2D::DrawSprite(BLImage* pImage, const BLRect& drawRect, const BLRectI& spriteRect, const BLRgba32& color, float rotateAngle)
{
    if(!pImage) {
        return;
    }

    if(color.a() == 0) {
        return;
    }

    //m_context.set_comp_op(BL_COMP_OP_SRC_OVER);

    bool restore = false;
    if(rotateAngle != 0.0f) {
        restore = true;
        m_context.save();

        float cX = drawRect.x + drawRect.h * 0.5;
        float cY = drawRect.y + drawRect.h * 0.5;
        
        m_context.translate(cX, cY);
        m_context.rotate(rotateAngle * M_PI / 180.0f);
        m_context.translate(-cX, -cY);
    }
    
    if (color != BLRgba32(255,255,255,255))
    {
        BLImage tinted = TintSprite(pImage, spriteRect, color);
        m_context.blit_image(drawRect, tinted);
    }
    else
    {
        m_context.blit_image(drawRect, *pImage, spriteRect);
    }

    if(restore) {
        m_context.restore();
    }
}

void Renderer2D::DrawText(BLFont* pFont, const BLPoint& origin, const string& text, const BLRgba32& color, float size, float rotateAngle)
{
    if(!pFont) {
        return;
    }

    m_context.set_comp_op(BL_COMP_OP_SRC_OVER);

    pFont->set_size(size);

    bool restore = false;
    if(color != BLRgba32(0, 0, 0, 255)) {
        restore = true;

        m_context.save();
        m_context.set_fill_style(color);
    }

    m_context.fill_utf8_text(origin, *pFont, text.c_str());

    if(restore) {
        m_context.restore();
    }
}

void Renderer2D::DrawGTText(BLFont* pFont, const BLPoint& origin, const std::string& text, float size) {
    if (!pFont) return;

    m_context.set_comp_op(BL_COMP_OP_SRC_OVER);
    BLRgba32 currentColor = sColorMap['0'];
    pFont->set_size(size);

    const char* str = text.c_str();
    BLFontMetrics fm = pFont->metrics();
    float drawY = origin.y + float(fm.ascent);

    while (str && *str) {
        const char* nl = strchr(str, '\n');
        usize lineLen = nl ? (usize)(nl - str) : strlen(str);

        float drawX = origin.x;

        for (size_t i = 0; i < lineLen; ++i) {
            char c = str[i];

            if (c == '`' && i + 1 < lineLen) {
                char colorCode = str[i + 1];
                if(sColorMap.find(colorCode) != sColorMap.end()) {
                    currentColor = sColorMap[colorCode];
                }
                ++i;
                continue;
            }

            BLGlyphBuffer gb;
            gb.set_utf8_text(&c, 1);
            pFont->shape(gb);

            BLTextMetrics tm;
            pFont->get_text_metrics(gb, tm);

            m_context.fill_glyph_run(BLPoint(drawX, drawY), *pFont, gb.glyph_run(), currentColor);
            drawX += tm.advance.x;
        }

        drawY += float(fm.ascent + fm.descent + fm.line_gap);
        str = nl ? nl + 1 : nullptr;
    }
}

bool Renderer2D::WriteToFile(const string& path)
{
    End();
    return m_surface.write_to_file(path.c_str()) == BL_SUCCESS;
}

float Renderer2D::GetTextWidth(BLFont* pFont, const string& text, float size)
{
    if(!pFont) {
        return 0.0f;
    }

    float oldFontSize = pFont->size();
    pFont->set_size(size);

    BLGlyphBuffer gb;
    gb.set_utf8_text(text.c_str(), text.size());
    pFont->shape(gb);

    BLTextMetrics tm;
    pFont->get_text_metrics(gb, tm);

    pFont->set_size(oldFontSize);
    return tm.advance.x;
}

float Renderer2D::GetTextHeight(BLFont* pFont, float size)
{
    if(!pFont) {
        return 0.0f;
    }

    float fontOldSize = pFont->size();
    pFont->set_size(size);

    BLFontMetrics fm = pFont->metrics();
    float textHeight = float(fm.ascent + fm.descent + fm.line_gap);

    pFont->set_size(fontOldSize);
    return textHeight;
}

BLImage Renderer2D::TintSprite(BLImage* src, const BLRectI& rect, const BLRgba32& color)
{
    BLImage out(rect.w, rect.h, BL_FORMAT_PRGB32);

    BLImageData srcData;
    src->get_data(&srcData);

    BLImageData dstData;
    out.get_data(&dstData);

    float rT = color.r() / 255.0f;
    float gT = color.g() / 255.0f;
    float bT = color.b() / 255.0f;
    float aT = color.a() / 255.0f;

    for(int32 y = 0; y < rect.h; ++y)
    {
        uint8* srcRow = (uint8*)srcData.pixel_data + (y + rect.y) * srcData.stride;
        uint8* dstRow = (uint8*)dstData.pixel_data + y * dstData.stride;

        for(int32 x = 0; x < rect.w; ++x)
        {
            uint8* spx = srcRow + (x + rect.x) * 4;
            uint8* dpx = dstRow + x * 4;

            float r = spx[2] / 255.0f;
            float g = spx[1] / 255.0f;
            float b = spx[0] / 255.0f;
            
            float srcA = spx[3] / 255.0f;
            
            r *= rT;
            g *= gT;
            b *= bT;
            srcA *= aT;
            
            dpx[0] = uint8(b * srcA * 255.0f);
            dpx[1] = uint8(g * srcA * 255.0f);
            dpx[2] = uint8(r * srcA * 255.0f);
            dpx[3] = uint8(srcA * 255.0f);
        }
    }

    return out;
}