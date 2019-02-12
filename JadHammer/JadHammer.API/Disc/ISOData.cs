using BizHawk.Emulation.DiscSystem;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace JadHammer.API
{
	/// <summary>
	/// ISO data obtained directly from the volume descriptor (if available)
	/// </summary>
	public class ISOData
	{
		public string AbstractFileIdentifier { get; set; }
		public string ApplicationIdentifier { get; set; }
		public string BibliographicalFileIdentifier { get; set; }
		public string CopyrightFileIdentifier { get; set; }
		public string DataPreparerIdentifier { get; set; }
		public DateTime? EffectiveDateTime { get; set; }
		public DateTime? ExpirationDateTime { get; set; }
		public DateTime? LastModifiedDateTime { get; set; }
		public int NumberOfSectors { get; set; }
		public int PathTableSize { get; set; }
		public string PublisherIdentifier { get; set; }
		public string Reserved { get; set; }
		public ISONodeRecord RootDirectoryRecord { get; set; }
		public int SectorSize { get; set; }
		public string SystemIdentifier { get; set; }
		public int Type { get; set; }
		public DateTime? VolumeCreationDate { get; set; }
		public string VolumeIdentifier { get; set; }
		public int VolumeSequenceNumber { get; set; }
		public string VolumeSetIdentifier { get; set; }
		public int VolumeSetSize { get; set; }
		public Dictionary<string, ISONode> ISOFiles { get; set; }

		public static ISOData Populate(ISOVolumeDescriptor vd)
		{
			if (vd == null)
				return null;

			ISOData i = new ISOData();

			// strings
			i.AbstractFileIdentifier = System.Text.Encoding.Default.GetString(vd.AbstractFileIdentifier).TrimEnd('\0', ' ');
			i.ApplicationIdentifier = System.Text.Encoding.Default.GetString(vd.ApplicationIdentifier).TrimEnd('\0', ' ');
			i.BibliographicalFileIdentifier = System.Text.Encoding.Default.GetString(vd.BibliographicalFileIdentifier).TrimEnd('\0', ' ');
			i.CopyrightFileIdentifier = System.Text.Encoding.Default.GetString(vd.CopyrightFileIdentifier).TrimEnd('\0', ' ');
			i.DataPreparerIdentifier = System.Text.Encoding.Default.GetString(vd.DataPreparerIdentifier).TrimEnd('\0', ' ');
			i.PublisherIdentifier = System.Text.Encoding.Default.GetString(vd.PublisherIdentifier).TrimEnd('\0', ' ');
			i.Reserved = System.Text.Encoding.Default.GetString(vd.Reserved).Trim('\0');
			i.SystemIdentifier = System.Text.Encoding.Default.GetString(vd.SystemIdentifier).TrimEnd('\0', ' ');
			i.VolumeIdentifier = System.Text.Encoding.Default.GetString(vd.VolumeIdentifier).TrimEnd('\0', ' ');
			i.VolumeSetIdentifier = System.Text.Encoding.Default.GetString(vd.VolumeSetIdentifier).TrimEnd('\0', ' ');

			// ints
			i.NumberOfSectors = vd.NumberOfSectors;
			i.PathTableSize = vd.PathTableSize;
			i.SectorSize = vd.SectorSize;
			i.Type = vd.Type;
			i.VolumeSequenceNumber = vd.VolumeSequenceNumber;

			// datetimes
			i.EffectiveDateTime = TextConverters.ParseDiscDateTime(TextConverters.TruncateLongString(System.Text.Encoding.Default.GetString(vd.EffectiveDateTime.ToArray()).Trim(), 12));
			i.ExpirationDateTime = TextConverters.ParseDiscDateTime(TextConverters.TruncateLongString(System.Text.Encoding.Default.GetString(vd.ExpirationDateTime.ToArray()).Trim(), 12));
			i.LastModifiedDateTime = TextConverters.ParseDiscDateTime(TextConverters.TruncateLongString(System.Text.Encoding.Default.GetString(vd.LastModifiedDateTime.ToArray()).Trim(), 12));
			i.VolumeCreationDate = TextConverters.ParseDiscDateTime(TextConverters.TruncateLongString(System.Text.Encoding.Default.GetString(vd.VolumeCreationDateTime.ToArray()).Trim(), 12));

			// other
			i.RootDirectoryRecord = vd.RootDirectoryRecord;

			return i;
		}
	}
}
