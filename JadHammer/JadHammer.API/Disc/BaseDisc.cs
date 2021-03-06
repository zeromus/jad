﻿using System;
using System.Linq;
using BizHawk.Emulation.DiscSystem;

namespace JadHammer.API
{
	public abstract class BaseDisc : IDisposable
	{
		/// <summary>
		/// The detected input format
		/// </summary>
		public abstract InputDiscType InputType { get; }

		/// <summary>
		/// Loads the specified file into the DiscSystem
		/// </summary>
		/// <returns></returns>
		protected abstract bool _LoadDisc();

		/// <summary>
		/// The BizHawk.DiscSystem disc object
		/// </summary>
		public Disc MountedDisc = null;

		/// <summary>
		/// An ISO object for the disc (if this is available)
		/// </summary>
		public ISOFile ISODisc = new ISOFile();

		/// <summary>
		/// Human readable volume descriptor data
		/// </summary>
		public ISOData ISOParsedData => ISOData.Populate(ISODisc.VolumeDescriptors.FirstOrDefault());

		/// <summary>
		/// BizHawk's platform detection result
		/// </summary>
		public DiscType DetectedDiscPlatform = DiscType.UnknownFormat;

		/// <summary>
		/// Filepath passed in at instantiation
		/// </summary>
		public string FilePath;

		/// <summary>
		/// Attempts to output the mounted disc in the specified format
		/// </summary>
		/// <param name="fPath"></param>
		/// <param name="outputType"></param>
		/// <returns></returns>
		public bool EgestDisc(string fPath, OutputDiscType outputType)
		{
			return OutputBase.Run(fPath, outputType, this);
		}

		/// <summary>
		/// Serves as a constructor/initializer whilst detecting the input disc type
		/// </summary>
		/// <returns>NULL if detection or initialization fails</returns>
		public static BaseDisc IngestDisc(string filePath)
		{
			BaseDisc bd = null;
			
			if (bd == null) bd = CueDisc.DoFileDetection(filePath);
			if (bd == null) bd = CcdDisc.DoFileDetection(filePath);
			if (bd == null) bd = JadDisc.DoFileDetection(filePath);
			if (bd == null) bd = JacDisc.DoFileDetection(filePath);
			if (bd == null) bd = MdsDisc.DoFileDetection(filePath);
			if (bd == null) bd = ChdDisc.DoFileDetection(filePath);

			if (bd != null)
			{
				if (!bd._LoadDisc())
					return null;
			}

			return bd;
		}

		public void Dispose()
		{
			MountedDisc?.Dispose();
		}
	}
}
