using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace JpegTest
{
    class Program
    {
        static void Main(string[] args)
        {
            var d1 = System.IO.File.ReadAllBytes("d:/1.bin");
            var img = JpegCodec.JpegConvert.Convert(d1);
        }
    }
}
