using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using JadHammer.Jad;

namespace TestApp
{
	class Program
	{
		private static JadStream jStream;

		static void Main(string[] args)
		{
			var a = LibJad.jadStaticInit();
			var b = LibJad.jadstd_OpenStdio(ref jStream, "test.jad", "rb");

			Console.ReadKey();
		}
	}
}
