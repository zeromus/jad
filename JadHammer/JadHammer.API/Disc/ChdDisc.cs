using System;
using System.Diagnostics;
using System.IO;

namespace JadHammer.API
{
	/// <summary>
	/// Mame CHD
	/// </summary>
	public class ChdDisc : BaseDisc
	{
		/// <summary>
		/// The detected input format
		/// </summary>
		public override InputDiscType InputType => InputDiscType.CHD;

		/// <summary>
		/// Loads the specified file into the DiscSystem
		/// </summary>
		/// <returns></returns>
		protected override bool _LoadDisc()
		{
			throw new NotImplementedException("CHD loading not yet implemented");
		}

		/// <summary>
		/// Format detection and initialization based on file contents (not file extension)
		/// </summary>
		/// <param name="filePath"></param>
		/// <returns></returns>
		public static ChdDisc DoFileDetection(string filePath)
		{
			try
			{
				using (var fs = File.OpenRead(filePath))
				{
					using (var br = new BinaryReader(fs))
					{
						if (fs.Length < 0x100)
							throw new Exception("Header is too small for a CHD file");

						byte[] magicBytes = br.ReadBytes(8);
						string magicStr = System.Text.Encoding.Default.GetString(magicBytes);

						if (magicStr.Contains("MComprHD"))
						{
							// TODO: throw an exception if this is not a disc
							// TODO: also, check version and other things
							var bd = new ChdDisc
							{
								FilePath = filePath
							};
							return bd;
						}
						else
						{
							throw new Exception("CHD magic string not detected");
						}
					}
				}
			}
			catch (Exception e)
			{
				Debug.WriteLine("CHD detection failed: " + e);
				return null;
			}
		}
	}
}
