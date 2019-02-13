using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using BizHawk.Emulation.DiscSystem;
using JadHammer.Jad;

namespace JadHammer.API
{
	public class OutputJad : OutputBase
	{
		/// <summary>
		/// Attempts to write the mounted disk out to the specified format
		/// </summary>
		public override bool Run()
		{
			try
			{
				var d = Disc.MountedDisc;

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

				// uncompressed sector data

				List<JadSector> Sectors = new List<JadSector>();

				var buf2448 = new byte[2448];
				DiscSectorReader dsr = new DiscSectorReader(d);
				int nLBA = d.Session1.LeadoutLBA;
				for (int lba = 0; lba < nLBA; lba++)
				{
					dsr.ReadLBA_2448(lba, buf2448, 0);

					Sectors.Add(new JadSector
					{
						entire = buf2448
					});

					Debug.WriteLine("Processing Sector: " + lba);
				}

				int numSectors = Sectors.Count;

				//create JadCreationParams
				JadCreationParams jcp = new JadCreationParams();
				jcp.toc = toc;
				jcp.numSectors = numSectors;
				//jcp.allocator =		// not sure?
				//jcp.callback =		// not sure?
				
				// pass the everything to libjad *somehow*

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
