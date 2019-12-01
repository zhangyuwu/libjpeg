#pragma once

using namespace System;

namespace JpegCodec {
    public ref class ImageData
    {
    public:
        property int Width;
        property int Height;
        property int MaxColor;
        property array<System::Byte>^ Data;
    };

	public ref class JpegConvert
	{
    public:
        static ImageData^ Convert(array<System::Byte>^ bytes);
	};
}
