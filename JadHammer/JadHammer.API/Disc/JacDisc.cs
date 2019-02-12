using System;
using System.Diagnostics;
using System.IO;

namespace JadHammer.API
{
	/// <summary>
	/// JAC
	/// </summary>
	public class JacDisc : BaseDisc
	{
		/// <summary>
		/// The detected input format
		/// </summary>
		public override InputDiscType InputType => InputDiscType.JAC;

		/// <summary>
		/// Loads the specified file into the DiscSystem
		/// </summary>
		/// <returns></returns>
		protected override bool _LoadDisc()
		{
			throw new NotImplementedException("JAC loading not yet implemented");
		}

		/// <summary>
		/// Format detection and initialization based on file contents (not file extension)
		/// </summary>
		/// <param name="filePath"></param>
		/// <returns></returns>
		public static JacDisc DoFileDetection(string filePath)
		{
			try
			{
				using (var fs = File.OpenRead(filePath))
				{
					using (var br = new BinaryReader(fs))
					{
						if (fs.Length < 0x670)
							throw new Exception("Header is too small for a JAC file");

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
								throw new Exception("Found JAD when looking for JAC");
							}
							else
							{
								//JAC compressed
								var bd = new JacDisc
								{
									FilePath = filePath
								};
								return bd;
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
				Debug.WriteLine("JAC detection failed: " + e);
				return null;
			}
		}
	}
}
