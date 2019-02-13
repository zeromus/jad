using System;
using System.Runtime.InteropServices;

namespace JadHammer.Jad
{
	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct JadStream
	{
		public IntPtr opaque;
		public IntPtr read;
		public IntPtr write;
		public IntPtr seek;
		public IntPtr get;
		public IntPtr put;
		public IntPtr flush;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct JadAllocator
	{
		public IntPtr opaque;
		public IntPtr alloc;
		public IntPtr realloc;
		public IntPtr free;
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
	
	public struct JadSector
	{
		/*
		[MarshalAs(UnmanagedType.ByValArray, SizeConst = 2352), FieldOffset(0)]
		public byte[] data;
		[MarshalAs(UnmanagedType.ByValArray, SizeConst = 96), FieldOffset(2352)]
		public byte[] subcode;
		[MarshalAs(UnmanagedType.ByValArray, SizeConst = 2448), FieldOffset(0)]
		public byte[] entire;
		*/

		[MarshalAs(UnmanagedType.ByValArray, SizeConst = 2448)]
		public byte[] entire;

		public byte[] data
		{
			get
			{
				byte[] dArr = new byte[2352];
				Array.Copy(entire, dArr, 2352);
				return dArr;
			}
			set
			{
				Array.Copy(value, entire, 2352);
			}
		}

		public byte[] subcode
		{
			get
			{
				byte[] sArr = new byte[96];
				Array.Copy(entire, 2352, sArr, 0, 96);
				return sArr;
			}
			set
			{
				Array.Copy(value, 0, entire, 2352, 96);
			}
		}
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
		public byte q_status;
		public byte q_tno;
		public byte q_index;
		public byte zero;
		public byte padding1;
		public byte padding2;
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
		//[MarshalAs(UnmanagedType.LPStruct)]
		public IntPtr callback;
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
