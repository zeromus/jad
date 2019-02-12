﻿using System;
using System.Diagnostics;
using System.IO;

namespace JadHammer.API
{
	/// <summary>
	/// CCD/IMG/SUB
	/// </summary>
	public class CcdDisc : BaseDisc
	{
		/// <summary>
		/// The detected input format
		/// </summary>
		public override InputDiscType InputType => InputDiscType.CCD;

		/// <summary>
		/// Loads the specified file into the DiscSystem
		/// </summary>
		/// <returns></returns>
		protected override bool _LoadDisc()
		{
			throw new NotImplementedException("CCD loading not yet implemented");
		}

		/// <summary>
		/// Format detection and initialization based on file contents (not file extension)
		/// </summary>
		/// <param name="filePath"></param>
		/// <returns></returns>
		public static CcdDisc DoFileDetection(string filePath)
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

						if (s.Contains("[CloneCD]") &&
						    s.Contains("Version=") &&
						    s.Contains("[Disc]") &&
						    s.Contains("[Session "))
						{
							CcdDisc bd = new CcdDisc();
							bd.FilePath = filePath;
							return bd;
						}
						else
						{
							throw new Exception("Not enough valid CCD keywords found in the first " + len + " characters");
						}
					}
				}
			}
			catch (Exception e)
			{
				Debug.WriteLine("CCD detection failed: " + e);
				return null;
			}
		}
	}
}
