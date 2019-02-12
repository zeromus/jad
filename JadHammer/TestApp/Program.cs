﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using JadHammer.Jad;
using JadHammer.API;

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
			string testCue = @"G:\_Emulation\PSX\iso\Speedball 2100 (USA)\Speedball 2100 (USA).cue";
			//var discObj = BaseDisc.IngestDisc(testCue);

			string testCcd = @"G:\_Emulation\PSX\iso\Resident Evil - Director's Cut - Dual Shock Ver. (USA)\Resident Evil - Director's Cut - Dual Shock Ver. (USA).ccd";
			//var discObj2 = BaseDisc.IngestDisc(testCcd);

			string testMds = @"G:\_Emulation\PSX\iso\LEGORACERS.mds";
			//var discObj3 = BaseDisc.IngestDisc(testMds);

			string testOther = @"G:\_Emulation\PCFX\Games\Super Power League FX.cue";
			var discObj4 = BaseDisc.IngestDisc(testOther);
			discObj4.EgestDisc(@"G:\_Emulation\PCFX\Games\SPLFX.jad", OutputDiscType.JAD);

			Console.ReadKey();

		/*
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
			*/
			Console.ReadKey();
		}
	}
}
