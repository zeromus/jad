using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
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

		public ProgressReporter Progress = new ProgressReporter();

		/// <summary>
		/// Signs whether the output file should be a compressed JAC (or not)
		/// </summary>
		public bool IsJac { get; set; }

		/// <summary>
		/// Clumsy override - set file extension based on JAC or JAD
		/// </summary>
		public override string FilePath
		{
			get => _filePath;
			set
			{
				if (Path.HasExtension(value))
				{
					if (IsJac)
						_filePath = Path.ChangeExtension(value, ".jac");
					else
						_filePath = Path.ChangeExtension(value, ".jad");
				}
				else
				{
					if (IsJac)
						_filePath = value + ".jac";
					else
					{
						_filePath = value + ".jad";
					}
				}
			}
		}
		private string _filePath;

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

		private uint numSectors = 0;

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
				var tocEntries = new JadSubchannelQ[101];
				toc.header = header;

				for (int i = 0; i < d.RawTOCEntries.Count; i++)
				{
					var entry = d.RawTOCEntries[i];

					tocEntries[i] = new JadSubchannelQ
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
				
				numSectors = (uint)d.Session1.LeadoutLBA;

				// static init
				jadErrStatus = LibJad.jadStaticInit();
				if (jadErrStatus != LibJad.JAD_OK)
					throw new ApplicationException("LibJad Error: jadStaticInit: " + Enum.GetName(typeof(JadStatus), jadErrStatus));

				// allocator
				JadAllocator allocator = new JadAllocator();
				jadErrStatus = LibJad.jadstd_OpenAllocator(ref allocator);
				if (jadErrStatus != LibJad.JAD_OK)
					throw new ApplicationException("LibJad Error: jadstd_OpenAllocator: " + Enum.GetName(typeof(JadStatus), jadErrStatus));

				// jadstream
				JadStream stream = new JadStream();
				jadErrStatus = LibJad.jadstd_OpenStdio(ref stream, FilePath, "wb");
				if (jadErrStatus != LibJad.JAD_OK)
					throw new ApplicationException("LibJad Error: jadstd_OpenStdio: " + Enum.GetName(typeof(JadStatus), jadErrStatus));

				// JadCreationParams
				fixed (JadSubchannelQ* pTocEntries = &tocEntries[0])
				{
					toc.entries = pTocEntries;
					JadCreationParams jcp = new JadCreationParams
					{
						toc = &toc,
						numSectors = numSectors,
						allocator = &allocator,
						callback = Marshal.GetFunctionPointerForDelegate(SectorReadCallback)
					};

					// context
					JadContext context = new JadContext();

					jadErrStatus = LibJad.jadCreate(ref context, ref jcp, ref allocator);
					if (jadErrStatus != LibJad.JAD_OK)
						throw new ApplicationException("LibJad Error: jadCreate: " + Enum.GetName(typeof(JadStatus), jadErrStatus));

					// dump
					//context.stream = stream;
					jadErrStatus = LibJad.jadDump(ref context, ref stream, IsJac ? 1 : 0);
					if (jadErrStatus != LibJad.JAD_OK)
						throw new ApplicationException("LibJad Error: jadDump: " + Enum.GetName(typeof(JadStatus), jadErrStatus));
				}

				// close
				jadErrStatus = LibJad.jadstd_CloseStdio(ref stream);
				if (jadErrStatus != LibJad.JAD_OK && jadErrStatus != LibJad.JAD_EOF)
					throw new ApplicationException("LibJad Error: jadstd_CloseStdio: " + Enum.GetName(typeof(JadStatus), jadErrStatus));

				jadErrStatus = LibJad.jadstd_CloseAllocator(ref allocator);
				if (jadErrStatus != LibJad.JAD_OK)
					throw new ApplicationException("LibJad Error: jadstd_CloseAllocator: " + Enum.GetName(typeof(JadStatus), jadErrStatus));

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

			Progress.HeaderText = IsJac ? "Dumping compressed JAC sectors:" : "Dumping raw JAD sectors:";
		}

		/// <summary>
		/// Read sector delegate
		/// </summary>
		/// <param name="opaque"></param>
		/// <param name="sectorNumber"></param>
		/// <param name="sectorBuffer"></param>
		/// <param name="subCodeBuffer"></param>
		[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
		private delegate int JadReadCallbackDelegate(IntPtr opaque, int sectorNumber, IntPtr* sectorBuffer, IntPtr* subCodeBuffer);

		/// <summary>
		/// The actual read sector callback method
		/// </summary>
		/// <param name="opaque"></param>
		/// <param name="sectorNumber"></param>
		/// <param name="sectorBuffer"></param>
		/// <param name="subCodeBuffer"></param>
		int MyJadCreateReadCallback(IntPtr opaque, int sectorNumber, IntPtr* sectorBuffer, IntPtr* subCodeBuffer)
		{
			byte[] buf2448 = new byte[2448];
			DSR.ReadLBA_2448(sectorNumber, buf2448, 0);
			Array.Copy(buf2448, SectorPinned.Data, 2352);
			Array.Copy(buf2448, 2352, SubcodePinned.Data, 0, 96);

			*sectorBuffer = SectorPinned.Ptr;
			*subCodeBuffer = SubcodePinned.Ptr;

			Progress.SignalProgress(0, numSectors, (uint) sectorNumber);

			return 0;
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
