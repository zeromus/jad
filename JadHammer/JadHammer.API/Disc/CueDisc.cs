using System;
using System.Diagnostics;
using System.IO;

namespace JadHammer.API
{
	/// <summary>
	/// CUE/BIN etc.
	/// </summary>
	public class CueDisc : BaseDisc
	{
		/// <summary>
		/// The detected input format
		/// </summary>
		public override InputDiscType InputType => InputDiscType.CUE;

		/// <summary>
		/// Loads the specified file into the DiscSystem
		/// </summary>
		/// <returns></returns>
		protected override bool _LoadDisc()
		{
			throw new NotImplementedException("CUE loading not yet implemented");
		}

		/// <summary>
		/// Format detection and initialization based on file contents (not file extension)
		/// </summary>
		/// <param name="filePath"></param>
		/// <returns></returns>
		public static CueDisc DoFileDetection(string filePath)
		{
			try
			{
				using (var fs = File.OpenRead(filePath))
				{
					using (var sr = new StreamReader(fs))
					{
						long len = 500;
						if (sr.BaseStream.Length < len)
							len = sr.BaseStream.Length;
						char[] buffer = new char[len];
						sr.Read(buffer, 0, (int)len);
						string s = new string(buffer).ToUpper();

						if (s.Contains("FILE ") &&
						    s.Contains("TRACK ") &&
						    s.Contains("INDEX ") &&
						    (s.Contains(" WAVE") ||
						     s.Contains(" MP3") ||
						     s.Contains(" AUDIO") ||
						     s.Contains(" BINARY") ||
						     s.Contains(" AIFF")))
						{
							CueDisc bd = new CueDisc();
							bd.FilePath = filePath;
							return bd;
						}
						else
						{
							throw new Exception("Not enough valid CUE keywords found in the first " + len + " characters");
						}
					}
				}
			}
			catch (Exception e)
			{
				Debug.WriteLine("CUE detection failed: " + e);
				return null;
			}
		}
	}
}
