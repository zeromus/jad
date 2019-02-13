using System;
using System.IO;
using System.Runtime.InteropServices;

namespace JadHammer.Jad
{
	public static class LibJad
	{
		private const string dllName = "jad.dll";

		public const int JAD_OK = 0;
		public const int JAD_EOF = 1;
		public const int JAD_ERROR = 2;
		public const int JAD_ERROR_FILE_NOT_FOUND = 3;

		#region jad

		/// <summary>
		/// performs necessary static initialization
		/// </summary>
		/// <returns></returns>
		[DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
		public static extern int jadStaticInit();

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

		#endregion

		#region std

		[DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
		public static extern int jadstd_OpenStdio(ref JadStream stream, string fname, string mode);

		[DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
		public static extern int jadstd_CloseStdio(ref JadStream stream);

		[DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
		public static extern int jadstd_OpenAllocator(ref JadAllocator allocator);

		[DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
		public static extern int jadstd_CloseAllocator(ref JadAllocator allocator);

		#endregion
	}
}

