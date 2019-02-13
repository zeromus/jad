using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace JadHammer.API
{
	/// <summary>
	/// Base class of output operations
	/// </summary>
	public abstract class OutputBase : IDisposable
	{
		/// <summary>
		/// Mounted disc passed in via constructor
		/// </summary>
		protected BaseDisc Disc = null;

		/// <summary>
		/// Destination file path
		/// </summary>
		public virtual string FilePath { get; set; }

		/// <summary>
		/// Attempts to output to the specified format
		/// </summary>
		/// <param name="filePath"></param>
		/// <param name="disc"></param>
		/// <returns></returns>
		public abstract bool Run();

		/// <summary>
		/// Attempts to write the mounted disk out to the specified format
		/// </summary>
		/// <param name="filePath"></param>
		/// <param name="type"></param>
		/// <param name="disc"></param>
		/// <returns></returns>
		public static bool Run(string filePath, OutputDiscType type, BaseDisc disc)
		{
			OutputBase ob = null;

			try
			{
				switch (type)
				{
					case OutputDiscType.CCD:
						ob = new OutputCcd
						{
							Disc = disc,
							FilePath = filePath
						};
						ob.Run();
						return true;
					case OutputDiscType.JAD:
						ob = new OutputJad
						{
							IsJac = false,
							Disc = disc,
							FilePath = filePath
						};
						ob.Run();
						return true;
					case OutputDiscType.JAC:
						ob = new OutputJad
						{
							IsJac = true,
							Disc = disc,
							FilePath = filePath
						};
						ob.Run();
						return true;
					default:
						return false;
				}
			}
			catch (Exception e)
			{
				return false;
			}
		}

		public virtual void Dispose()
		{

		}
	}
}
