using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices.WindowsRuntime;
using BizHawk.Emulation.DiscSystem;
using JadHammer.Jad;

namespace JadHammer.API
{
	/// <summary>
	/// JAD
	/// </summary>
	public class JadDisc : BaseDisc
	{
		/// <summary>
		/// The detected input format
		/// </summary>
		public override InputDiscType InputType => InputDiscType.JAD;

		/// <summary>
		/// Loads the specified file into the DiscSystem
		/// </summary>
		/// <returns></returns>
		protected override bool _LoadDisc()
		{
			try
			{
				var dmj = new DiscMountJob
				{
					IN_DiscInterface = DiscInterface.BizHawk,
					IN_FromPath = FilePath
				};
				dmj.Run();
				MountedDisc = dmj.OUT_Disc;

				var dider = new DiscIdentifier(MountedDisc);
				DetectedDiscPlatform = dider.DetectDiscType();

				var discView = EDiscStreamView.DiscStreamView_Mode1_2048;
				if (MountedDisc.TOC.Session1Format == SessionFormat.Type20_CDXA)
					discView = EDiscStreamView.DiscStreamView_Mode2_Form1_2048;

				ISODisc.Parse(new DiscStream(MountedDisc, discView, 0));

				return true;
			}
			catch (Exception e)
			{
				Debug.WriteLine("ERROR mounting disc: " + e);
				return false;
			}

			/*
			// basic test to verify that JadHammer can use libjad.Open() without error (although there is currently not much in libjad.open)
			try
			{
				int jadErrStatus = 0;

				// create the new jadStream
				using (var jsh = new JadStreamHandler(FilePath))
				{
					jsh.Init();

					// static init
					jadErrStatus = LibJad.jadStaticInit();
					if (jadErrStatus != LibJad.JAD_OK)
						throw new ApplicationException("LibJad Error: jadStaticInit: " + Enum.GetName(typeof(JadStatus), jadErrStatus));

					// allocator
					JadAllocator allocator = new JadAllocator();
					jadErrStatus = LibJad.jadstd_OpenAllocator(ref allocator);
					if (jadErrStatus != LibJad.JAD_OK)
						throw new ApplicationException("LibJad Error: jadstd_OpenAllocator: " + Enum.GetName(typeof(JadStatus), jadErrStatus));

					// context
					JadContext context = new JadContext
					{
						allocator = allocator,
						stream = jsh.JStream
					};

					// open
					jadErrStatus = LibJad.jadOpen(ref context, ref jsh.JStream, ref allocator);		// appears to work for what its worth
				}
			}
			catch (Exception e)
			{
				Debug.WriteLine(e);
				return false;
			}
			*/

			return true;
		}

		/// <summary>
		/// Format detection and initialization based on file contents (not file extension)
		/// </summary>
		/// <param name="filePath"></param>
		/// <returns></returns>
		public static JadDisc DoFileDetection(string filePath)
		{
			try
			{
				using (var fs = File.OpenRead(filePath))
				{
					using (var br = new BinaryReader(fs))
					{
						if (fs.Length < 0x670)
							throw new Exception("Header is too small for a JAD file");

						byte[] magicBytes = br.ReadBytes(8);
						byte terminator = magicBytes[7];
						byte[] shorter = new byte[7];
						Array.Copy(magicBytes, shorter, 7);
						string magicStr = System.Text.Encoding.Default.GetString(shorter);

						if (magicStr == "JADJAC!" && terminator == 0x01)
						{
							br.ReadBytes(8);

							var flags = br.ReadUInt32();

							// TODO: better flag handling logic
							if (flags == 0)
							{
								// JAD uncompressed
								var bd = new JadDisc
								{
									FilePath = filePath
								};
								return bd;
							}
							else
							{
								throw new Exception("Found JAC when looking for JAD");
							}
						}
						else
						{
							throw new Exception("JADJAC! magic string not detected");
						}
					}
				}
			}
			catch (Exception e)
			{
				Debug.WriteLine("JAD detection failed: " + e);
				return null;
			}
		}
	}
}
