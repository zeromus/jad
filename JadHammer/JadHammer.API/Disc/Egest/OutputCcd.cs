using System;
using System.Collections.Generic;
using System.Diagnostics;
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
		public override bool Run()
		{
			try
			{
				CCD_Format.Dump(Disc.MountedDisc, FilePath);
				return true;
			}
			catch (Exception e)
			{
				Debug.WriteLine(e);
				return false;
			}
		}
	}
}
