using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using BizHawk.Emulation.DiscSystem;
using BizHawk.Emulation.DiscSystem.CUE;

namespace JadHammer.API
{
	public class OutputCcd : OutputBase
	{
		/// <summary>
		/// Attempts to write the mounted disk out to the specified format
		/// </summary>
		/// <param name="filePath"></param>
		/// <param name="disc"></param>
		/// <returns></returns>
		public override bool Run()
		{
			try
			{
				CCD_Format.Dump(Disc.MountedDisc, FilePath);
			}
			catch (Exception e)
			{
				Console.WriteLine(e);
				return false;
			}

			throw new NotImplementedException();
		}
	}
}
