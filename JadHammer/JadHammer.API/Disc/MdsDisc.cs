using BizHawk.Emulation.DiscSystem;
using System;
using System.Diagnostics;
using System.IO;

namespace JadHammer.API
{
	/// <summary>
	/// CCD/IMG/SUB
	/// </summary>
	public class MdsDisc : BaseDisc
	{
		/// <summary>
		/// The detected input format
		/// </summary>
		public override InputDiscType InputType => InputDiscType.MDS;

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
		}

		/// <summary>
		/// Format detection and initialization based on file contents (not file extension)
		/// </summary>
		/// <param name="filePath"></param>
		/// <returns></returns>
		public static MdsDisc DoFileDetection(string filePath)
		{
			try
			{
				using (var fs = File.OpenRead(filePath))
				{
					using (var br = new BinaryReader(fs))
					{
						if (fs.Length < 0x100)
							throw new Exception("Header is too small for an MDS file");

						byte[] magicBytes = br.ReadBytes(16);
						string magicStr = System.Text.Encoding.Default.GetString(magicBytes);

						if (magicStr.Contains("MEDIA DESCRIPTOR"))
						{
							byte[] version = br.ReadBytes(2);

							if (version[0] > 1 || version[0] == 0)
							{
								throw new Exception("Unsupported MDS version detected: " + version[0] + "." + version[1]);
							}

							var bd = new MdsDisc
							{
								FilePath = filePath
							};
							return bd;
						}
						else
						{
							throw new Exception("MDS magic string not detected");
						}
					}
				}
			}
			catch (Exception e)
			{
				Debug.WriteLine("MDS detection failed: " + e);
				return null;
			}
		}
	}
}
