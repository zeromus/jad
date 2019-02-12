using System;
using System.Diagnostics;
using System.IO;

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
			throw new NotImplementedException("JAD loading not yet implemented");
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
