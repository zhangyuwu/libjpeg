#include "stdafx.h"
#include "JpegCodec.h"
#include <stdio.h>
#include <exception>

extern void jpegconv(FILE * in_stream, FILE * out_stream, int colortrafo, bool upsample);
extern FILE * tmpmfile(const char * prefix);

namespace JpegCodec
{
    int get_bytes_per_pixel(int max_color) {
        int n = 0;
        while (max_color > 0) {
            max_color /= 0x100;
            n++;
        }
        return n;
    }

    void write_bytes_to_stream(FILE * stream, array<System::Byte>^ bytes) {
        for (auto i = 0; i < bytes->Length; i++) {
            unsigned char c = bytes[i];
            fwrite(&c, 1, 1, stream);
        }
    }

    ImageData^ ReadPPM(FILE * stream) {
        const int LINE_SIZE = 1024;
        char line[LINE_SIZE];
        int width, height, max_color;
        auto imgdata = gcnew ImageData();
        fgets(line, LINE_SIZE, stream);
        fscanf(stream, "%d %d %d\n", &width, &height, &max_color);
        imgdata->Width = width;
        imgdata->Height = height;
        imgdata->MaxColor = max_color;
        
        auto bytes_per_pixel = get_bytes_per_pixel(max_color);
        imgdata->Data = gcnew array<System::Byte>(width * height * bytes_per_pixel);
        unsigned char c;
        int i = 0;

        auto x = ftell(stream);
        while (!feof(stream)) {
            if (fread(&c, 1, 1, stream) > 0) {
                imgdata->Data[i++] = c;
            }
        }
        return imgdata;
    }

    ImageData^ JpegConvert::Convert(array<System::Byte>^ bytes) {
        try {
            auto in = tmpmfile("in");
            auto out = tmpmfile("out");
            write_bytes_to_stream(in, bytes);
            rewind(in);
            jpegconv(in, out, 1, true);
            rewind(out);
            auto imgdata = ReadPPM(out);
            fclose(in);
            fclose(out);
            return imgdata;
        }
        catch (std::exception &ex) {
            throw gcnew System::IO::IOException();
        }
    }
}