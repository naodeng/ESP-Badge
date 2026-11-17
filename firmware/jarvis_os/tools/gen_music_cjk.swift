// Generate the dynamic music-metadata fallback from LVGL's bundled Source Han Sans SC.
// Run from any directory after ESP-IDF components have been downloaded:
//   swift firmware/jarvis_os/tools/gen_music_cjk.swift
import Foundation
import CoreFoundation
import CoreText
import CoreGraphics

let fontBox = 24
let fontSize: CGFloat = 18
let projectRoot = URL(fileURLWithPath: #filePath)
    .deletingLastPathComponent()
    .deletingLastPathComponent()
let sourceURL = projectRoot.appendingPathComponent("managed_components/lvgl__lvgl/scripts/generators/built_in_font/SourceHanSansSC-Normal.otf")
let outputURL = projectRoot.appendingPathComponent("main/font_music_cjk_24.c")

guard FileManager.default.fileExists(atPath: sourceURL.path) else {
    FileHandle.standardError.write("Source Han Sans font not found at \(sourceURL.path)\n".data(using: .utf8)!)
    exit(1)
}
guard let descriptors = CTFontManagerCreateFontDescriptorsFromURL(sourceURL as CFURL) as? [CTFontDescriptor],
      let descriptor = descriptors.first else {
    FileHandle.standardError.write("Unable to read Source Han Sans font descriptor\n".data(using: .utf8)!)
    exit(1)
}
let font = CTFontCreateWithFontDescriptor(descriptor, fontSize, nil)
let ascent = CTFontGetAscent(font)
let descent = CTFontGetDescent(font)

// Decode the common Chinese double-byte plane through Core Foundation's GBK codec.
// The selected A1-F7 lead-byte range covers the GB2312 character repertoire.
let gbk = CFStringEncoding(CFStringEncodings.GBK_95.rawValue)
let encoding = String.Encoding(rawValue: CFStringConvertEncodingToNSStringEncoding(gbk))
var characters = Set<UnicodeScalar>()
for lead in 0xA1...0xF7 {
    for trail in 0xA1...0xFE {
        let bytes = Data([UInt8(lead), UInt8(trail)])
        if let decoded = String(data: bytes, encoding: encoding),
           decoded.unicodeScalars.count == 1,
           let scalar = decoded.unicodeScalars.first {
            characters.insert(scalar)
        }
    }
}
let glyphs = characters.sorted { $0.value < $1.value }
guard !glyphs.isEmpty, glyphs.count <= Int(UInt16.max) else {
    FileHandle.standardError.write("Invalid CJK glyph count: \(glyphs.count)\n".data(using: .utf8)!)
    exit(1)
}

func render(_ scalar: UnicodeScalar) -> [UInt8] {
    var gray = [UInt8](repeating: 0, count: fontBox * fontBox)
    let space = CGColorSpaceCreateDeviceGray()
    guard let context = CGContext(data: nil, width: fontBox, height: fontBox,
                                  bitsPerComponent: 8, bytesPerRow: fontBox,
                                  space: space, bitmapInfo: CGImageAlphaInfo.none.rawValue) else {
        return gray
    }
    context.setAllowsAntialiasing(true)
    context.setShouldSmoothFonts(false)
    let attributes = [kCTFontAttributeName: font,
                      kCTForegroundColorAttributeName: CGColor(gray: 1, alpha: 1)] as CFDictionary
    let line = CTLineCreateWithAttributedString(
        CFAttributedStringCreate(nil, String(scalar) as CFString, attributes))
    let advance = CGFloat(CTLineGetTypographicBounds(line, nil, nil, nil))
    context.textPosition = CGPoint(x: (CGFloat(fontBox) - advance) / 2,
                                   y: (CGFloat(fontBox) - ascent - descent) / 2 + descent)
    CTLineDraw(line, context)
    if let data = context.data {
        let pixels = data.bindMemory(to: UInt8.self, capacity: fontBox * fontBox)
        for index in gray.indices { gray[index] = pixels[index] }
    }
    return gray
}

var bitmap: [UInt8] = []
var descriptorsText = "    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0}, /* id 0 */\n"
var bitmapText = ""
for scalar in glyphs {
    let pixels = render(scalar)
    let visible = pixels.indices.filter { pixels[$0] >= 32 }
    let minX = visible.map { $0 % fontBox }.min() ?? 0
    let minY = visible.map { $0 / fontBox }.min() ?? 0
    let maxX = (visible.map { $0 % fontBox }.max() ?? -1) + 1
    let maxY = (visible.map { $0 / fontBox }.max() ?? -1) + 1
    let width = maxX - minX
    let height = maxY - minY
    var cropped: [UInt8] = []
    if width > 0 && height > 0 {
        for y in minY..<maxY {
            for x in minX..<maxX { cropped.append(pixels[y * fontBox + x]) }
        }
    }
    let bitmapIndex = bitmap.count
    var glyphBitmap: [UInt8] = []
    for index in stride(from: 0, to: cropped.count, by: 4) {
        var packed: UInt8 = 0
        for pixel in 0..<min(4, cropped.count - index) {
            let level = UInt8((UInt16(cropped[index + pixel]) * 3 + 127) / 255)
            packed |= level << UInt8(6 - pixel * 2)
        }
        glyphBitmap.append(packed)
    }
    bitmap.append(contentsOf: glyphBitmap)
    bitmapText += String(format: "    /* U+%04X */\n    ", scalar.value)
    bitmapText += glyphBitmap
        .enumerated().map { index, byte in
            String(format: "0x%02x,%@", byte, (index % 16 == 15) ? "\n    " : " ")
        }.joined()
    bitmapText += "\n\n"
    descriptorsText += "    {.bitmap_index = \(bitmapIndex), .adv_w = \(Int(fontSize * 16)), .box_w = \(width), .box_h = \(height), .ofs_x = \(minX), .ofs_y = \(height == 0 ? 0 : fontBox - maxY)},\n"
}

let first = glyphs.first!.value
let last = glyphs.last!.value
let offsets = glyphs.map { String($0.value - first) }.joined(separator: ", ")
let source = """
// Generated from Source Han Sans SC, common CJK repertoire, 18px/2bpp.
#include "lvgl.h"

static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
\(bitmapText)};

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
\(descriptorsText)};

static const uint16_t unicode_list_0[] = { \(offsets) };
static const lv_font_fmt_txt_cmap_t cmaps[] = {
    {.range_start = \(first), .range_length = \(last - first + 1), .glyph_id_start = 1,
     .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL,
     .list_length = \(glyphs.count), .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY},
};
static const lv_font_fmt_txt_dsc_t font_dsc = {
    .glyph_bitmap = glyph_bitmap, .glyph_dsc = glyph_dsc, .cmaps = cmaps,
    .kern_dsc = NULL, .kern_scale = 0, .cmap_num = 1, .bpp = 2,
    .kern_classes = 0, .bitmap_format = 0,
};
const lv_font_t font_music_cjk_24 = {
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,
    .line_height = \(fontBox), .base_line = 0, .subpx = LV_FONT_SUBPX_NONE,
    .underline_position = 0, .underline_thickness = 0, .dsc = &font_dsc,
    .fallback = NULL, .user_data = NULL,
};
"""
let cleanSource = source.replacingOccurrences(
    of: #"(?m)[ \t]+$"#, with: "", options: .regularExpression)
try cleanSource.write(to: outputURL, atomically: true, encoding: .utf8)
print("\(outputURL.path): \(glyphs.count) glyphs, bitmap \(bitmap.count) bytes")
