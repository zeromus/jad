using System;
using System.Runtime.InteropServices;

namespace JadHammer.Jad
{
	/// <summary>
	/// Utility class that handles pinning and unpinning of byte arrays
	/// </summary>
	public class PinnedBuffer : IDisposable
	{
		public GCHandle Handle { get; }
		public byte[] Data { get; private set; }

		public IntPtr Ptr => Handle.AddrOfPinnedObject();

		public PinnedBuffer(byte[] bytes)
		{
			Data = bytes;
			Handle = GCHandle.Alloc(bytes, GCHandleType.Pinned);
		}

		public void Dispose()
		{
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		protected virtual void Dispose(bool disposing)
		{
			if (disposing)
			{
				Handle.Free();
				Data = null;
			}
		}
	}
}
