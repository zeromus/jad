using System;
using System.Collections.Generic;
using System.Dynamic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using JadHammer.Jad;

namespace JadHammer.API
{
	/// <summary>
	/// Will handle JadStream callbacks and such (once jad reading is implemented in libjad)
	/// This creates a new JadStream object which can be passed to libjad.jadOpen()
	/// Currently no provision for stream->opaque changing (would this ever happen??)
	/// </summary>
	public unsafe class JadStreamHandler : IDisposable
	{
		/// <summary>
		/// The created JadStream object
		/// </summary>
		public JadStream JStream;

		/// <summary>
		/// The stored filepath
		/// </summary>
		private readonly string FilePath;

		/// <summary>
		/// The working FileStream
		/// </summary>
		private FileStream FStream;

		public JadStreamHandler(string filePath, FileMode mode = FileMode.Open)
		{
			FilePath = filePath;

			if (!File.Exists(filePath))
			{
				throw new IOException("Input file does not exist");
			}

			FStream = new FileStream(FilePath, mode);
			JStream = new JadStream();
		}

		/// <summary>
		/// Must be called after instantiation to init callbacks
		/// </summary>
		public void Init()
		{
			StreamReadCallback = new JStreamReadDelegate(JStreamReadCallback);
			StreamWriteCallback = new JStreamWriteDelegate(JStreamWriteCallback);
			StreamSeekCallback = new JStreamSeekDelegate(JStreamSeekCallback);
			StreamGetCallback = new JStreamGetDelegate(JStreamGetCallback);
			StreamPutCallback = new JStreamPutDelegate(JStreamPutCallback);

			JStream.read = Marshal.GetFunctionPointerForDelegate(StreamReadCallback);
			JStream.write = Marshal.GetFunctionPointerForDelegate(StreamWriteCallback);
			JStream.seek = Marshal.GetFunctionPointerForDelegate(StreamSeekCallback);
			JStream.get = Marshal.GetFunctionPointerForDelegate(StreamGetCallback);
			JStream.put = Marshal.GetFunctionPointerForDelegate(StreamPutCallback);
		}

		#region Callbacks

		/// <summary>
		/// Read 'bytes' number of bytes from the stream and write into buffer
		/// </summary>
		/// <param name="buffer"></param>
		/// <param name="bytes"></param>
		/// <param name="stream"></param>
		/// <returns>The number of full items read (which may be less than 'bytes' if an error or EOF occured</returns>
		unsafe int JStreamReadCallback(ref IntPtr buffer, uint bytes, ref JadStream* stream)
		{
			byte[] buff = new byte[bytes];
			int res = 0;

			fixed (byte* p = buff)
			{
				res = FStream.Read(buff, 0, (int)bytes);
				buffer = (IntPtr) p;
			}

			return res;
		}

		/// <summary>
		/// Writes 'bytes' number of bytes from the buffer into the stream
		/// </summary>
		/// <param name="buffer"></param>
		/// <param name="bytes"></param>
		/// <param name="stream"></param>
		/// <returns></returns>
		unsafe int JStreamWriteCallback(ref IntPtr buffer, uint bytes, ref JadStream* stream)
		{
			byte[] buff = new byte[bytes];
			UnmanagedMemoryStream uStream = new UnmanagedMemoryStream((byte*)buffer, bytes); // this could be all kinds of wrong - needs testing once functionality is implemented in libjad.
			int res = 0;
			for (var i = 0; i < bytes; i++, res++)
			{
				var r = uStream.ReadByte();
				if (r == -1)
					break;
				buff[i] = (byte)r;
			}

			FStream.Write(buff, 0, res);
			return res;
		}

		/// <summary>
		/// Seeks to the specified offset based on origin
		/// </summary>
		/// <param name="stream"></param>
		/// <param name="offset"></param>
		/// <param name="origin"></param>
		/// <returns></returns>
		unsafe long JStreamSeekCallback(ref JadStream* stream, long offset, int origin)
		{
			SeekOrigin so = SeekOrigin.Current;
			if (origin == 0) so = SeekOrigin.Begin;
			else if (origin == 2) so = SeekOrigin.End;

			long res = FStream.Seek(offset, so);
			if (res < 0)
				return res;

			offset = FStream.Position;
			return offset;
		}

		/// <summary>
		/// Gets the next character from the stream
		/// </summary>
		/// <param name="stream"></param>
		/// <returns></returns>
		unsafe int JStreamGetCallback(ref JadStream* stream)
		{
			var res = FStream.ReadByte();
			if (res == -1)
				return LibJad.JAD_EOF;

			return res;
		}

		/// <summary>
		/// Writes the supplied byte to the stream
		/// </summary>
		/// <param name="stream"></param>
		/// <param name="value"></param>
		/// <returns></returns>
		unsafe int JStreamPutCallback(ref JadStream* stream, byte value)
		{
			FStream.WriteByte(value);
			return value;
		}

		#endregion

		#region Delegates

		protected virtual JStreamReadDelegate StreamReadCallback { get; set; }
		protected virtual JStreamWriteDelegate StreamWriteCallback { get; set; }
		protected virtual JStreamSeekDelegate StreamSeekCallback { get; set; }
		protected virtual JStreamGetDelegate StreamGetCallback { get; set; }
		protected virtual JStreamPutDelegate StreamPutCallback { get; set; }
		protected virtual JStreamFlushDelegate StreamFlushCallback { get; set; }	// looks like libjad doesnt even implement this yet

		/// <summary>
		/// the read callback for a jadStream
		/// </summary>
		/// <param name="buffer"></param>
		/// <param name="bytes"></param>
		/// <param name="stream"></param>
		/// <returns></returns>
		[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
		public unsafe delegate int JStreamReadDelegate(ref IntPtr buffer, uint bytes, ref JadStream* stream);
		/// <summary>
		/// the write callback for a jadStream
		/// </summary>
		/// <param name="buffer"></param>
		/// <param name="bytes"></param>
		/// <param name="stream"></param>
		/// <returns></returns>
		[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
		public unsafe delegate int JStreamWriteDelegate(ref IntPtr buffer, uint bytes, ref JadStream* stream);
		/// <summary>
		/// the seek callback for a jadStream. like fseek, but returns ftell. (0,SEEK_CUR) should work as ftell and work even on fundamentally non-seekable streams
		/// </summary>
		/// <param name="stream"></param>
		/// <param name="offset"></param>
		/// <param name="origin"></param>
		/// <returns></returns>
		[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
		public unsafe delegate long JStreamSeekDelegate(ref JadStream* stream, long offset, int origin);
		/// <summary>
		/// the codec get callback for a jadStream, like getc, but no provision for ferror is provided
		/// </summary>
		/// <param name="stream"></param>
		/// <returns></returns>
		[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
		public unsafe delegate int JStreamGetDelegate(ref JadStream* stream);
		/// <summary>
		/// the codec put callback for a jadStream, like putc, but no provision for ferror is provided
		/// </summary>
		/// <param name="stream"></param>
		/// <param name="val"></param>
		/// <returns></returns>
		[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
		public unsafe delegate int JStreamPutDelegate(ref JadStream* stream, byte val);
		/// <summary>
		/// just used for codec flushing
		/// </summary>
		/// <param name="stream"></param>
		/// <returns></returns>
		[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
		public unsafe delegate int JStreamFlushDelegate(ref JadStream* stream);

		#endregion

		public void Dispose()
		{
			FStream.Dispose();
			FStream = null;
		}
	}
}
