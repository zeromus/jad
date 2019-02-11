using System;
using System.IO;
using System.Runtime.InteropServices;

namespace Jad
{
	public static class LibJad
	{
		private const string dllName = "jad.lib";

		/// <summary>
		/// performs necessary static initialization
		/// </summary>
		/// <returns></returns>
		[DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
		public static extern int jadStaticInit();

<<<<<<< HEAD
		[DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
		public static extern int jadstd_OpenStdio(ref JadStream stream, string fname, string mode);

		[DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
		public static extern int jadstd_CloseStdio(ref JadStream stream);

=======
>>>>>>> a1404af075b0307450ac106b30630a4ce0dc511d
		/// <summary>
		/// opens a jadContext, which gets its data from the provided stream (containing a jad/jac file) and allocator
		/// </summary>
		[DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
		public static extern int jadOpen(ref JadContext jad, ref JadStream stream, ref JadAllocator allocator);

		/// <summary>
		/// creates a jadContext based on the provided jadCreationParams, which describe the disc
		/// </summary>
		[DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
		public static extern int jadCreate(ref JadContext jad, ref JadCreationParams jcp, ref JadAllocator allocator);

		/// <summary>
		/// dumps the given jad to the output stream as a JAD or JAC file
		/// </summary>
		[DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
		public static extern int jadDump(ref JadContext jad, ref JadStream stream, int JACIT);

		/// <summary>
		/// closes a jadContext opened with jadOpen or jadCreate. do not call on unopened jadContexts
		/// </summary>
		[DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
		public static extern int jadClose(ref JadContext jad);
	}
}

