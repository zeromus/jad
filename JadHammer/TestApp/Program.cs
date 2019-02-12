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
		private static JadStream jStream = new JadStream();
		private static JadContext jContext = new JadContext();
		private static JadCreationParams jCreationParams = new JadCreationParams();
		private static JadAllocator jAllocator = new JadAllocator();

		static void Main(string[] args)
		{
			var a = LibJad.jadStaticInit();
			//var b = LibJad.jadstd_OpenStdio(ref jStream, "test.jad", "rb");

			jCreationParams = new JadCreationParams
			{
				numSectors = 100,
				toc = new JadTOC(),
				allocator = jAllocator,
			};
			jCreationParams.toc.header = new JadTocHeader
			{
				firstTrack = 1,
				lastTrack = 1,
				flags = 0
			};
			jCreationParams.toc.entries = new JadSubchannelQ[101];
			for (var i = 0; i < 101; i++)
			{
				jCreationParams.toc.entries[i] = new JadSubchannelQ();
				jCreationParams.toc.entries[i].q_index = (byte) i;
			}

			var c = LibJad.jadCreate(ref jContext, ref jCreationParams, ref jAllocator);

			Console.ReadKey();
		}
	}
}
