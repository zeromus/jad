using System;
using System.Text;
using System.IO;
using System.Globalization;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using BizHawk.Emulation.DiscSystem.CUE;
using JadHammer.Jad;

namespace BizHawk.Emulation.DiscSystem
{
    /// <summary>
    /// Parsing JAD files
    /// https://github.com/zeromus/jad
    /// </summary>
	public class JAD_Format
    {
        /// <summary>
        /// A loose representation of a JAD file
        /// </summary>
        public class JADFile
        {
            /// <summary>
            /// Full path to the MDS file
            /// </summary>
            public string JADPath;

			/// <summary>
			/// The JAD magic string
			/// </summary>
            public string MagicString;

			/// <summary>
			/// JAD version
			/// </summary>
			public uint Version;

			/// <summary>
			/// JAD header flags
			/// </summary>
			public uint Flags;

			/// <summary>
			/// Offset to metadata/tags
			/// </summary>
			public ulong MetadataOffset;

			/// <summary>
			/// Number of sectors stored in the file
			/// </summary>
			public uint NumSectors;

			/// <summary>
			/// The 4 byte toc header
			/// </summary>
			public JadTocHeader TOCHeader;

			/// <summary>
			/// The toc jadsubchannelq entries
			/// </summary>
			public List<JadSubchannelQ> TOCEntries = new List<JadSubchannelQ>(101);

			/// <summary>
			/// Offset to where the sector data starts
			/// </summary>
			public long SectorDataOffset;
        }

        public JADFile Parse(Stream stream)
        {
            EndianBitConverter bc = EndianBitConverter.CreateForLittleEndian();
            EndianBitConverter bcBig = EndianBitConverter.CreateForBigEndian();
            bool isDvd = false;

            JADFile aFile = new JADFile();

            aFile.JADPath = (stream as FileStream).Name;

            stream.Seek(0, SeekOrigin.Begin);

            // check whether the header in the mds file is long enough
            if (stream.Length < 0x670) throw new JADParseException("Malformed JAD format: The file header does not appear to be long enough.");

            // parse header
			byte[] arr8 = new byte[8];
			stream.Read(arr8, 0, 8);
			aFile.MagicString = Encoding.Default.GetString(arr8);
			arr8 = new byte[8];

			byte[] arr4 = new byte[4];
			stream.Read(arr4, 0, 4);
			aFile.Version = (uint)bc.ToInt32(arr4);

			stream.Read(arr4, 0, 4);
			aFile.Flags = (uint)bc.ToInt32(arr4);

			arr8 = new byte[8];
			stream.Read(arr8, 0, 8);
			aFile.MetadataOffset = (ulong) bc.ToInt64(arr8);

			arr4 = new byte[4];
			stream.Read(arr4, 0, 4);
			aFile.NumSectors = (uint) bc.ToInt32(arr4);

			JadTocHeader header = new JadTocHeader();
			header.firstTrack = (byte)stream.ReadByte();
			header.lastTrack = (byte)stream.ReadByte();
			header.flags = (byte)stream.ReadByte();
			header.reserved = (byte)stream.ReadByte();
			aFile.TOCHeader = header;

			// iterate through each q entry
			for (int i = 0; i < 101; i++)
			{
				byte[] qData = new byte[16];
				stream.Read(qData, 0, 16);

				JadSubchannelQ q = new JadSubchannelQ
				{
					q_timestamp = new JadTimestamp
					{
						MIN = qData[0],
						SEC = qData[1],
						FRAC = qData[3],
						_padding = qData[4]
					},
					q_apTimestamp = new JadTimestamp
					{
						MIN = qData[5],
						SEC = qData[6],
						FRAC = qData[7],
						_padding = qData[8]
					},
					q_crc = (ushort)bc.ToInt16(qData.Skip(8).Take(2).ToArray()),
					q_status = qData[10],
					q_tno = qData[11],
					q_index = qData[12],
					zero = qData[13],
					padding1 = qData[14],
					padding2 = qData[15]
				};

				aFile.TOCEntries.Add(q);
			}

			aFile.SectorDataOffset = stream.Position + 2448;

            return aFile;
        }

        public class JADParseException : Exception
        {
            public JADParseException(string message) : base(message) { }
        }
        

        public class LoadResults
        {
            public List<RawTOCEntry> RawTOCEntries;
            public JADFile ParsedJADFile;
            public bool Valid;
            public Exception FailureException;
            public string JadPath;
        }

        public static LoadResults LoadJADPath(string path)
        {
            LoadResults ret = new LoadResults();
            ret.JadPath = path;

            try
            {
                if (!File.Exists(path)) throw new JADParseException("Malformed JAD format: nonexistent JAD file!");

                JADFile jadf;
                using (var infJAD = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read))
                    jadf = new JAD_Format().Parse(infJAD);

                ret.ParsedJADFile = jadf;

                ret.Valid = true;
            }
            catch (JADParseException ex)
            {
                ret.FailureException = ex;
            }

            return ret;
        }

        /// <summary>
        /// Loads a JAD at the specified path to a Disc object
        /// </summary>
        public Disc LoadJADToDisc(string jadPath, DiscMountPolicy IN_DiscMountPolicy)
        {
            var loadResults = LoadJADPath(jadPath);
            if (!loadResults.Valid)
                throw loadResults.FailureException;

            Disc disc = new Disc();

            IBlob jadBlob = null;
            long jadLen = -1;

            var jPath = loadResults.JadPath;

			if (!File.Exists(jPath)) throw new JADParseException("Malformed JAD format: nonexistent JAD file");
			var jFile = new Disc.Blob_JadFile()
			{
				PhysicalPath = jPath,
				Offset = loadResults.ParsedJADFile.SectorDataOffset
			};

			jadLen = jFile.Length;
			jadBlob = jFile;

			disc.DisposableResources.Add(jadBlob);

			SS_JAD synth = new SS_JAD();

			var mdsf = loadResults.ParsedJADFile;
            
            //generate DiscTOCRaw items from the ones specified in the JAD file
            disc.RawTOCEntries = new List<RawTOCEntry>();
            foreach (var entry in mdsf.TOCEntries)
            {
				var q = new SubchannelQ
				{
					q_status = entry.q_status,
					q_tno = BCD2.FromBCD(entry.q_tno),
					q_index = BCD2.FromBCD(entry.q_index),
					min = BCD2.FromBCD(entry.q_timestamp.MIN),
					sec = BCD2.FromBCD(entry.q_timestamp.SEC),
					frame = BCD2.FromBCD(entry.q_timestamp.FRAC),
					zero = entry.zero,
					ap_min = BCD2.FromBCD(entry.q_apTimestamp.MIN),
					ap_sec = BCD2.FromBCD(entry.q_apTimestamp.SEC),
					ap_frame = BCD2.FromBCD(entry.q_apTimestamp.FRAC),
					q_crc = entry.q_crc
				};

				disc.RawTOCEntries.Add(new RawTOCEntry { QData = q });
            }

            // now build the sectors
            for (int i = 0; i < loadResults.ParsedJADFile.NumSectors; i++)
            {
	            disc._Sectors.Add(synth);
            }
            return disc;
        }

        class SS_JAD : ISectorSynthJob2448
        {
	        public void Synth(SectorSynthJob job)
	        {
		        var jadBlob = job.Disc.DisposableResources[0] as IBlob;

				// JAD should have everything we need (by design)

		        if ((job.Parts & ESectorSynthPart.UserAny) != 0)
		        {
					long ofs = job.LBA * 2352;
					jadBlob.Read(ofs, job.DestBuffer2448, 0, 2352);
				}

		        if ((job.Parts & (ESectorSynthPart.SubcodeAny)) != 0)
		        {
			        long ofs = job.LBA * 96;
			        jadBlob.Read(ofs, job.DestBuffer2448, 2352, 96);

			        //subcode comes to us deinterleved; we may still need to interleave it
			        if ((job.Parts & (ESectorSynthPart.SubcodeDeinterleave)) == 0)
			        {
				        SynthUtils.InterleaveSubcodeInplace(job.DestBuffer2448, job.DestOffset + 2352);
			        }
		        }
			}
        }

    } //class JAD_Format
}


