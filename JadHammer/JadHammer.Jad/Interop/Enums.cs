
namespace JadHammer.Jad
{
	public enum jadEnumControlQ
	{
		jadEnumControlQ_None = 0,

		jadEnumControlQ_StereoNoPreEmph = 0,
		jadEnumControlQ_StereoPreEmph = 1,
		jadEnumControlQ_MonoNoPreemph = 8,
		jadEnumControlQ_MonoPreEmph = 9,
		jadEnumControlQ_DataUninterrupted = 4,
		jadEnumControlQ_DataIncremental = 5,

		jadEnumControlQ_CopyProhibitedMask = 0,
		jadEnumControlQ_CopyPermittedMask = 2,
	}

	public enum jadEncodeSettings
	{
		jadEncodeSettings_None = 0,

		jadEncodeSettings_DirectCopy = 1
	}

	public enum jadFlags
	{
		jadFlags_None = 0,

		//whether the file is JAC subformat (compressed)
		jadFlags_JAC = 1,

		jadFlags_BigEndian = 2
	}

	public enum JadStatus
	{
		JAD_OK = 0,
		JAD_EOF = -1,
		JAD_ERROR = -2,
		JAD_ERROR_FILE_NOT_FOUND = -3,
	}
}
