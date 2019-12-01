/*************************************************************************

    This project implements a complete(!) JPEG (Recommendation ITU-T
    T.81 | ISO/IEC 10918-1) codec, plus a library that can be used to
    encode and decode JPEG streams.
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Note that only Profiles C and D of ISO/IEC 18477-7 are supported
    here. Check the JPEG XT reference software for a full implementation
    of ISO/IEC 18477-7.

    Copyright (C) 2012-2018 Thomas Richter, University of Stuttgart and
    Accusoft. (C) 2019 Thomas Richter, Fraunhofer IIS.

    This program is available under two licenses, GPLv3 and the ITU
    Software licence Annex A Option 2, RAND conditions.

    For the full text of the GPU license option, see README.license.gpl.
    For the full text of the ITU license option, see README.license.itu.

    You may freely select beween these two options.

    For the GPL option, please note the following:

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*************************************************************************/
/*
** This file includes the decompressor front-end of the
** command line interface. It doesn't do much except
** calling libjpeg.
**
** $Id: reconstruct.cpp,v 1.8 2017/11/28 13:08:03 thor Exp $
**
*/

///
/*************************************************************************

    This project implements a complete(!) JPEG (Recommendation ITU-T
    T.81 | ISO/IEC 10918-1) codec, plus a library that can be used to
    encode and decode JPEG streams.
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Note that only Profiles C and D of ISO/IEC 18477-7 are supported
    here. Check the JPEG XT reference software for a full implementation
    of ISO/IEC 18477-7.

    Copyright (C) 2012-2018 Thomas Richter, University of Stuttgart and
    Accusoft. (C) 2019 Thomas Richter, Fraunhofer IIS.

    This program is available under two licenses, GPLv3 and the ITU
    Software licence Annex A Option 2, RAND conditions.

    For the full text of the GPU license option, see README.license.gpl.
    For the full text of the ITU license option, see README.license.itu.

    You may freely select beween these two options.

    For the GPL option, please note the following:

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*************************************************************************/
/*
** This header provides the main function.
** This main function is only a demo, it is not part of the libjpeg code.
** It is here to serve as an entry point for the command line image
** compressor.
**
** $Id: main.cpp,v 1.215 2018/07/20 06:18:56 thor Exp $
**
*/

/// Includes
#include "std/stdio.hpp"
#include "std/stdlib.hpp"
#include "std/string.hpp"
#include "std/math.hpp"
#include "tools/environment.hpp"
#include "tools/traits.hpp"
#include "interface/hooks.hpp"
#include "interface/tagitem.hpp"
#include "interface/parameters.hpp"
#include "interface/jpeg.hpp"
#include "tools/numerics.hpp"
#include "cmd/bitmaphook.hpp"
#include "cmd/filehook.hpp"
#include "cmd/iohelpers.hpp"
#include "cmd/tmo.hpp"
#include "cmd/defaulttmoc.hpp"
#include "cmd/encodec.hpp"
#include "cmd/encodeb.hpp"
#include "cmd/encodea.hpp"
#include "cmd/reconstruct.hpp"
///

bool oznew = false;

/// Defines
#define FIX_BITS 13
///

/// Parse the subsampling factors off.
void ParseSubsamplingFactors(UBYTE *sx, UBYTE *sy, const char *sub, int cnt) {
    char *end;

    do {
        int x = strtol(sub, &end, 0);
        *sx++ = x;
        if (*end == 'x' || *end == 'X') {
            sub = end + 1;
            int y = strtol(sub, &end, 0);
            *sy++ = y;
            if (*end != ',')
                break;
            sub = end + 1;
        }
        else break;
    } while (--cnt);
}
///

/// ParseDouble
// Parse off a floating point value, or print an error
double ParseDouble(int &argc, char **&argv) {
    double v;
    char *endptr;

    if (argv[2] == NULL) {
        fprintf(stderr, "%s expected a numeric argument.\n", argv[1]);
        exit(25);
    }

    v = strtod(argv[2], &endptr);

    if (*endptr) {
        fprintf(stderr, "%s expected a numeric argument, not %s.\n", argv[1], argv[2]);
        exit(25);
    }

    argc -= 2;
    argv += 2;

    return v;
}
///

/// ParseInt
// Parse off an integer, return it, or print an error.
int ParseInt(int &argc, char **&argv) {
    long int v;
    char *endptr;

    if (argv[2] == NULL) {
        fprintf(stderr, "%s expected a numeric argument.\n", argv[1]);
        exit(25);
    }

    v = strtol(argv[2], &endptr, 0);

    if (*endptr) {
        fprintf(stderr, "%s expected a numeric argument, not %s.\n", argv[1], argv[2]);
        exit(25);
    }

    argc -= 2;
    argv += 2;

    return v;
}
///

/// ParseString
// Parse of a string argument, return it, or print an error
const char *ParseString(int &argc, char **&argv) {
    const char *v;

    if (argv[2] == NULL) {
        fprintf(stderr, "%s expects a string argument.\n", argv[1]);
        exit(25);
    }

    v = argv[2];

    argc -= 2;
    argv += 2;

    return v;
}
///

#include <exception>

/// Reconstruct
// This reconstructs an image from the given input file
// and writes the output ppm.
void jpegconv(FILE * in_stream, FILE * out_stream, int colortrafo, bool upsample) {
    if (in_stream == NULL || out_stream == NULL) {
        throw std::exception("Input/output stream is null");
    }

    struct JPG_Hook filehook(FileHook, in_stream);
    class JPEG *jpeg = JPEG::Construct(NULL);
    if (jpeg) {
        int ok = 1;
        struct JPG_TagItem tags[] = {
          JPG_PointerTag(JPGTAG_HOOK_IOHOOK,&filehook),
          JPG_PointerTag(JPGTAG_HOOK_IOSTREAM,in_stream),
          JPG_EndTag
        };

        if (ok && jpeg->Read(tags)) {
            UBYTE subx[4], suby[4];
            struct JPG_TagItem atags[] = {
              JPG_ValueTag(JPGTAG_IMAGE_PRECISION,0),
              JPG_ValueTag(JPGTAG_IMAGE_IS_FLOAT,false),
              JPG_ValueTag(JPGTAG_IMAGE_OUTPUT_CONVERSION,true),
              JPG_EndTag
            };
            struct JPG_TagItem itags[] = {
              JPG_ValueTag(JPGTAG_IMAGE_WIDTH,0),
              JPG_ValueTag(JPGTAG_IMAGE_HEIGHT,0),
              JPG_ValueTag(JPGTAG_IMAGE_DEPTH,0),
              JPG_ValueTag(JPGTAG_IMAGE_PRECISION,0),
              JPG_ValueTag(JPGTAG_IMAGE_IS_FLOAT,false),
              JPG_ValueTag(JPGTAG_IMAGE_OUTPUT_CONVERSION,true),
              JPG_ValueTag(JPGTAG_ALPHA_MODE,JPGFLAG_ALPHA_OPAQUE),
              JPG_PointerTag(JPGTAG_ALPHA_TAGLIST,atags),
              JPG_PointerTag(JPGTAG_IMAGE_SUBX,subx),
              JPG_PointerTag(JPGTAG_IMAGE_SUBY,suby),
              JPG_ValueTag(JPGTAG_IMAGE_SUBLENGTH,4),
              JPG_EndTag
            };
            if (jpeg->GetInformation(itags)) {
                ULONG width = itags->GetTagData(JPGTAG_IMAGE_WIDTH);
                ULONG height = itags->GetTagData(JPGTAG_IMAGE_HEIGHT);
                UBYTE depth = itags->GetTagData(JPGTAG_IMAGE_DEPTH);
                UBYTE prec = itags->GetTagData(JPGTAG_IMAGE_PRECISION);
                UBYTE aprec = 0;
                bool  pfm = itags->GetTagData(JPGTAG_IMAGE_IS_FLOAT) ? true : false;
                bool convert = itags->GetTagData(JPGTAG_IMAGE_OUTPUT_CONVERSION) ? true : false;
                bool doalpha = itags->GetTagData(JPGTAG_ALPHA_MODE, JPGFLAG_ALPHA_OPAQUE) ? true : false;
                bool apfm = false;
                bool aconvert = false;
                bool writepgx = false;
                const char * alpha = NULL;

                if (alpha && doalpha) {
                    aprec = atags->GetTagData(JPGTAG_IMAGE_PRECISION);
                    apfm = atags->GetTagData(JPGTAG_IMAGE_IS_FLOAT) ? true : false;
                    aconvert = atags->GetTagData(JPGTAG_IMAGE_OUTPUT_CONVERSION) ? true : false;
                }
                else {
                    doalpha = false; // alpha channel is ignored.
                }

                UBYTE bytesperpixel = sizeof(UBYTE);
                UBYTE pixeltype = CTYP_UBYTE;
                if (prec > 8) {
                    bytesperpixel = sizeof(UWORD);
                    pixeltype = CTYP_UWORD;
                }
                if (pfm && convert == false) {
                    bytesperpixel = sizeof(FLOAT);
                    pixeltype = CTYP_FLOAT;
                }

                UBYTE alphabytesperpixel = sizeof(UBYTE);
                UBYTE alphapixeltype = CTYP_UBYTE;
                if (aprec > 8) {
                    alphabytesperpixel = sizeof(UWORD);
                    alphapixeltype = CTYP_UWORD;
                }
                if (apfm && aconvert == false) {
                    alphabytesperpixel = sizeof(FLOAT);
                    alphapixeltype = CTYP_FLOAT;
                }

                UBYTE *amem = NULL;
                UBYTE *mem = (UBYTE *)malloc(width * 8 * depth * bytesperpixel);
                if (mem == NULL) {
                    throw std::exception("Memory allocation");
                }

                if (doalpha) {
                    amem = (UBYTE *)malloc(width * 8 * alphabytesperpixel); // only one component!
                    if (amem == NULL) {
                        throw std::exception("Memory allocation");
                    }
                }

                struct BitmapMemory bmm;
                bmm.bmm_pMemPtr = mem;
                bmm.bmm_pAlphaPtr = amem;
                bmm.bmm_ulWidth = width;
                bmm.bmm_ulHeight = height;
                bmm.bmm_usDepth = depth;
                bmm.bmm_ucPixelType = pixeltype;
                bmm.bmm_ucAlphaType = alphapixeltype;
                bmm.bmm_pTarget = out_stream;
                bmm.bmm_pAlphaTarget = (doalpha) ? (fopen(alpha, "wb")) : NULL;
                bmm.bmm_pSource = NULL;
                bmm.bmm_pAlphaSource = NULL;
                bmm.bmm_pLDRSource = NULL;
                bmm.bmm_bFloat = pfm;
                bmm.bmm_bAlphaFloat = apfm;
                bmm.bmm_bBigEndian = true;
                bmm.bmm_bAlphaBigEndian = true;
                bmm.bmm_bNoOutputConversion = !convert;
                bmm.bmm_bNoAlphaOutputConversion = !aconvert;
                bmm.bmm_bUpsampling = upsample;

                memset(bmm.bmm_PGXFiles, 0, sizeof(bmm.bmm_PGXFiles));

                if (depth != 1 && depth != 3)
                    writepgx = true;
                if (!upsample)
                    writepgx = true;
                //
                // If upsampling is enabled, the subsampling factors are
                // all implicitly 1.
                if (upsample) {
                    memset(subx, 1, sizeof(subx));
                    memset(suby, 1, sizeof(suby));
                }

                bmm.bmm_bWritePGX = writepgx;

                struct JPG_Hook bmhook(BitmapHook, &bmm);
                struct JPG_Hook alphahook(AlphaHook, &bmm);
                // pgx reconstruction is component by component since
                // we cannot interleave non-upsampled components of differing
                // subsampling factors.
                if (writepgx) {
                    int comp;
                    for (comp = 0; comp < depth; comp++) {
                        ULONG y = 0;
                        ULONG lastline;
                        ULONG thisheight = height;
                        struct JPG_TagItem tags[] = {
                          JPG_PointerTag(JPGTAG_BIH_HOOK,&bmhook),
                          JPG_PointerTag(JPGTAG_BIH_ALPHAHOOK,&alphahook),
                          JPG_ValueTag(JPGTAG_DECODER_MINY,y),
                          JPG_ValueTag(JPGTAG_DECODER_MAXY,y + (suby[comp] << 3) - 1),
                          JPG_ValueTag(JPGTAG_DECODER_UPSAMPLE,upsample),
                          JPG_ValueTag(JPGTAG_MATRIX_LTRAFO,colortrafo),
                          JPG_ValueTag(JPGTAG_DECODER_MINCOMPONENT,comp),
                          JPG_ValueTag(JPGTAG_DECODER_MAXCOMPONENT,comp),
                          JPG_EndTag
                        };

                        // Reconstruct now the buffered image, line by line. Could also
                        // reconstruct the image as a whole. What we have here is just a demo
                        // that is not necessarily the most efficient way of handling images.
                        do {
                            lastline = height;
                            if (lastline > y + (suby[comp] << 3))
                                lastline = y + (suby[comp] << 3);
                            tags[2].ti_Data.ti_lData = y;
                            tags[3].ti_Data.ti_lData = lastline - 1;
                            ok = jpeg->DisplayRectangle(tags);
                            y = lastline;
                        } while (y < thisheight && ok);
                    }
                    for (int i = 0; i < depth; i++) {
                        if (bmm.bmm_PGXFiles[i]) {
                            fclose(bmm.bmm_PGXFiles[i]);
                        }
                    }
                }
                else {
                    ULONG y = 0; // Just as a demo, run a stripe-based reconstruction.
                    ULONG lastline;
                    struct JPG_TagItem tags[] = {
                      JPG_PointerTag(JPGTAG_BIH_HOOK,&bmhook),
                      JPG_PointerTag(JPGTAG_BIH_ALPHAHOOK,&alphahook),
                      JPG_ValueTag(JPGTAG_DECODER_MINY,y),
                      JPG_ValueTag(JPGTAG_DECODER_MAXY,y + 7),
                      JPG_ValueTag(JPGTAG_DECODER_UPSAMPLE,upsample),
                      JPG_ValueTag(JPGTAG_MATRIX_LTRAFO,colortrafo),
                      JPG_EndTag
                    };
                    fprintf(bmm.bmm_pTarget, "P%c\n%d %d\n%d\n",
                        (pfm) ? ((depth > 1) ? 'F' : 'f') : ((depth > 1) ? ('6') : ('5')),
                        width, height, (pfm) ? (1) : ((1 << prec) - 1));

                    if (bmm.bmm_pAlphaTarget)
                        fprintf(bmm.bmm_pAlphaTarget, "P%c\n%d %d\n%d\n",
                        (apfm) ? ('f') : ('5'),
                            width, height, (apfm) ? (1) : ((1 << aprec) - 1));
                    //
                    // Reconstruct now the buffered image, line by line. Could also
                    // reconstruct the image as a whole. What we have here is just a demo
                    // that is not necessarily the most efficient way of handling images.
                    do {
                        lastline = height;
                        if (lastline > y + 8)
                            lastline = y + 8;
                        tags[2].ti_Data.ti_lData = y;
                        tags[3].ti_Data.ti_lData = lastline - 1;
                        ok = jpeg->DisplayRectangle(tags);
                        y = lastline;
                    } while (y < height && ok);
                }

                if (bmm.bmm_pAlphaTarget)
                    fclose(bmm.bmm_pAlphaTarget);

                if (amem)
                    free(amem);

                free(mem);
            }
            else ok = 0;
        }
        else ok = 0;

        if (!ok) {
            const char *error;
            int code = jpeg->LastError(error);
            fprintf(stderr, "reading a JPEG file failed - error %d - %s\n", code, error);
            throw std::exception("JPEG read file");
        }
        JPEG::Destruct(jpeg);
    }
    else {
        fprintf(stderr, "failed to construct the JPEG object");
        throw std::exception("JPEG construct");
    }
}
///
