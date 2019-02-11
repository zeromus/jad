using System.Runtime.InteropServices;

namespace Jad
{
	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct JadStream
	{
		public JadStreamRead read;
		public JadStreamWrite write;
		public JadStreamSeek seek;
		public JadStreamGet get;
		public JadStreamPut put;
		public JadStreamFlush flush;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct JadAllocator
	{
		public JadAllocatorAlloc alloc;
		public JadAllocatorRealloc realloc;
		public JadAllocatorFree free;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct JadTocHeader
	{
		public byte firstTrack;
		public byte lastTrack;
		public byte flags;
		public byte reserved;
	}

	/// <summary>
	/// a timestamp like the format stored on a disc
	/// </summary>
	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct JadTimestamp
	{
		public byte MIN;
		public byte SEC;
		public byte FRAC;
		public byte _padding;
	}

	[StructLayout(LayoutKind.Explicit)]
	public struct JadSector
	{
		[MarshalAs(UnmanagedType.ByValArray, SizeConst = 2352), FieldOffset(0)]
		public byte[] data;
		[MarshalAs(UnmanagedType.ByValArray, SizeConst = 96), FieldOffset(2352)]
		public byte[] subcode;
		[MarshalAs(UnmanagedType.ByValArray, SizeConst = 2448), FieldOffset(0)]
		public byte[] entire;
	}

	/// <summary>
	/// this layout does not reflect the layout of a subQ on disc; separate serialization functions are used to represent that
	/// </summary>
	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct JadSubchannelQ
	{
		public JadTimestamp q_timestamp;
		public JadTimestamp q_apTimestamp;
		public ushort q_crc;
		public byte q_tno;
		public byte q_index;
		public byte zero;
		[MarshalAs(UnmanagedType.ByValArray, SizeConst = 2)]
		public byte[] padding;
	}

	/// <summary>
	/// TOC
	/// </summary>
	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct JadTOC
	{
		public JadTocHeader header;
		[MarshalAs(UnmanagedType.ByValArray, SizeConst = 101)]
		public JadSubchannelQ[] entries;
	}

	/// <summary>
	/// the params struct used for jadCreate
	/// </summary>
	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct JadCreationParams
	{
		public JadTOC toc;
		public JadAllocator allocator;
		public int numSectors;
		public JadCreateReadCallback callback;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct JadContext
	{
		public JadStream stream;
		public JadAllocator allocator;
		public uint flags;
		public uint numSectors;
		public JadCreationParams createParams;
	}
}
