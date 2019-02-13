using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using BizHawk.Emulation.DiscSystem;
using JadHammer.Jad;

namespace JadHammer.API
{
	public unsafe class OutputJad : OutputBase
	{
		private DiscSectorReader DSR { get; set; }

		/// <summary>
		/// Callback object
		/// </summary>
		private readonly JadReadCallbackDelegate SectorReadCallback;

		/// <summary>
		/// Working sector buffer
		/// </summary>
		private byte[] SectorBuffer = new byte[2352];

		/// <summary>
		/// Pinned sector buffer class
		/// </summary>
		private PinnedBuffer SectorPinned;

		/// <summary>
		/// Working subcode buffer
		/// </summary>
		private byte[] SubCodeBuffer = new byte[96];

		/// <summary>
		/// Pinned subcode buffer class
		/// </summary>
		private PinnedBuffer SubcodePinned;

		/// <summary>
		/// Attempts to write the mounted disk out to the specified format
		/// </summary>
		public override bool Run()
		{
			try
			{
				var d = Disc.MountedDisc;
				DSR = new DiscSectorReader(d);
				int jadErrStatus = 0;

				// header and toc
				JadTocHeader header = new JadTocHeader();
				header.firstTrack = (byte)d.TOC.FirstRecordedTrackNumber;
				header.lastTrack = (byte)d.TOC.LastRecordedTrackNumber;
				header.flags = 0; // not sure what this is
				header.reserved = 0;

				JadTOC toc = new JadTOC();
				toc.header = header;
				toc.entries = new JadSubchannelQ[101];

				for (int i = 0; i < d.RawTOCEntries.Count; i++)
				{
					var entry = d.RawTOCEntries[i];

					toc.entries[i] = new JadSubchannelQ
					{
						q_index = entry.QData.q_index.BCDValue,
						q_apTimestamp = new JadTimestamp
						{
							MIN = entry.QData.ap_min.BCDValue,
							FRAC = entry.QData.ap_frame.BCDValue,
							SEC = entry.QData.ap_sec.BCDValue,
							_padding = 0 // what is this?
						},
						q_timestamp = new JadTimestamp
						{
							MIN = entry.QData.min.BCDValue,
							FRAC = entry.QData.frame.BCDValue,
							SEC = entry.QData.sec.BCDValue,
							_padding = 0 // what is this?
						},
						padding1 = 0,
						padding2 = 0,
						q_crc = entry.QData.q_crc,
						q_tno = entry.QData.q_tno.BCDValue,
						q_status = (byte)(entry.QData.ADR + ((int)entry.QData.CONTROL << 4)),
						zero = 0
					};
				}
				
				int numSectors = d.Session1.LeadoutLBA;

				// allocator
				JadAllocator allocator = new JadAllocator();
				jadErrStatus = LibJad.jadstd_OpenAllocator(ref allocator);
				if (jadErrStatus != 0)
					throw new ApplicationException("LibJad Error: jadstd_OpenAllocator: " + ((JadStatus)jadErrStatus).ToString());

				// jadstream
				JadStream stream = new JadStream();
				jadErrStatus = LibJad.jadstd_OpenStdio(ref stream, FilePath, "wb");
				if (jadErrStatus != 0)
					throw new ApplicationException("LibJad Error: jadstd_OpenStdio: " + ((JadStatus)jadErrStatus).ToString());

				// JadCreationParams
				JadCreationParams jcp = new JadCreationParams
				{
					toc = toc,
					numSectors = numSectors,
					allocator = allocator,
					callback = Marshal.GetFunctionPointerForDelegate(SectorReadCallback)
				};

				// context
				JadContext context = new JadContext();

				// copying jcp manually to context because jadCreate doesnt appear to do this for some reason??
				context.allocator = allocator;
				context.stream = stream;
				context.createParams = jcp;
				context.numSectors = (uint)jcp.numSectors;

				jadErrStatus = LibJad.jadCreate(ref context, ref jcp, ref allocator);
				if (jadErrStatus != 0)
					throw new ApplicationException("LibJad Error: jadCreate: " + ((JadStatus) jadErrStatus).ToString());

				// dump
				// the following currently throws a:
				// "System.AccessViolationException: 'Attempted to read or write protected memory. This is often an indication that other memory is corrupt.'"
				jadErrStatus = LibJad.jadDump(ref context, ref stream, 0);
				if (jadErrStatus != 0)
					throw new ApplicationException("LibJad Error: jadDump: " + ((JadStatus) jadErrStatus).ToString());

				// close
				jadErrStatus = LibJad.jadstd_CloseStdio(ref stream);
				if (jadErrStatus != 0)
					throw new ApplicationException("LibJad Error: jadstd_CloseStdio: " + ((JadStatus)jadErrStatus).ToString());

				jadErrStatus = LibJad.jadstd_CloseAllocator(ref allocator);
				if (jadErrStatus != 0)
					throw new ApplicationException("LibJad Error: jadstd_CloseAllocator: " + ((JadStatus)jadErrStatus).ToString());

				return true;
			}
			catch (Exception e)
			{
				Debug.WriteLine(e);
				return false;
			}
		}

		/// <summary>
		/// Constructor
		/// </summary>
		public OutputJad()
		{
			// pinned buffers
			SectorPinned = new PinnedBuffer(SectorBuffer);
			SubcodePinned = new PinnedBuffer(SubCodeBuffer);

			// read sector callback
			SectorReadCallback = new JadReadCallbackDelegate(MyJadCreateReadCallback);
		}

		/// <summary>
		/// Read sector delegate
		/// </summary>
		/// <param name="opaque"></param>
		/// <param name="sectorNumber"></param>
		/// <param name="sectorBuffer"></param>
		/// <param name="subCodeBuffer"></param>
		private delegate void JadReadCallbackDelegate(IntPtr opaque, int sectorNumber, IntPtr* sectorBuffer, IntPtr* subCodeBuffer);

		/// <summary>
		/// The actual read sector callback method
		/// </summary>
		/// <param name="opaque"></param>
		/// <param name="sectorNumber"></param>
		/// <param name="sectorBuffer"></param>
		/// <param name="subCodeBuffer"></param>
		void MyJadCreateReadCallback(IntPtr opaque, int sectorNumber, IntPtr* sectorBuffer, IntPtr* subCodeBuffer)
		{
			DSR.ReadLBA_2352(sectorNumber, SectorBuffer, 0);
			DSR.ReadLBA_2448(sectorNumber, SubCodeBuffer, 2352);

			*sectorBuffer = SectorPinned.Ptr;
			*subCodeBuffer = SubcodePinned.Ptr;
		}

		/// <summary>
		/// Disposal
		/// </summary>
		public override void Dispose()
		{
			DSR = null;
			SectorPinned.Dispose();
			SubcodePinned.Dispose();
		}
	}
}
