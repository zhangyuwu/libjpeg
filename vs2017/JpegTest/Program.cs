using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace JpegTest
{
    public class TickCounter : IDisposable
    {
        public string Name { get; set; }

        public DateTime TimeStart { get; set; }

        public DateTime TimeStop { get; set; }

        public TickCounter(string name = null)
        {
            var stackTrace = new System.Diagnostics.StackTrace();
            var caller = stackTrace.GetFrame(1).GetMethod().Name;
            Name = name ?? caller;
            TimeStart = DateTime.Now;
        }

        public void Dispose()
        {
            TimeStop = DateTime.Now;
            var span = TimeStop - TimeStart;
            Console.WriteLine($"Time cost {span} for {Name}()");
        }

        public static void Measure(Action action)
        {
            using (new TickCounter()) {
                action();
            }
        }
    }

    class Program
    {
        static void Main(string[] args)
        {
            using (new TickCounter()) {
                var d1 = System.IO.File.ReadAllBytes("d:/1.bin");
                var img = JpegCodec.JpegConvert.Convert(d1);
            }
        }
    }
}
