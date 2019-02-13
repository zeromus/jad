using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace JadHammer.API
{
	/// <summary>
	/// A simple progress reporting thing
	/// </summary>
	public class ProgressReporter : IDisposable
	{
		public int UpdateInterval = 500;	// milliseconds for now
		public string HeaderText;

		private bool _firstLine = true;

		private Stopwatch _stopWatch = new Stopwatch();

		private OutputLocation _outputLocation;
		private OutputStyle _outputStyle;

		public ProgressReporter(OutputLocation outputLocation = OutputLocation.Console, 
			OutputStyle outputStyle = OutputStyle.CurrentOfMax,
			int updateInterval = 500,
			string headerText = null)
		{
			_outputLocation = outputLocation;
			_outputStyle = outputStyle;
			UpdateInterval = updateInterval;
			HeaderText = string.IsNullOrWhiteSpace(headerText) ? string.Empty : headerText;

			_stopWatch.Start();
		}

		private uint _min;
		private uint _max;
		private uint _current;

		public void SignalProgress(uint min, uint max, uint current)
		{
			var ts = _stopWatch.Elapsed;
			if (ts.TotalMilliseconds > UpdateInterval)
			{
				_stopWatch.Stop();
				_stopWatch.Reset();

				_max = max;
				_min = min;
				_current = current;

				switch (_outputLocation)
				{
					case OutputLocation.Console:
						if (_firstLine)
						{
							OutputToConsole(HeaderText);
							_firstLine = false;
						}
						OutputToConsole(null);
						break;
					case OutputLocation.Debug:
						if (_firstLine)
						{
							OutputToDebug(HeaderText);
							_firstLine = false;
						}
						OutputToDebug(null);
						break;
				}

				_stopWatch.Start();
			}
		}

		private void OutputToConsole(string headerText = null)
		{
			if (!string.IsNullOrWhiteSpace(headerText))
			{
				Console.WriteLine(headerText);
			}
			else
			{
				var s = BuildOutputString();
				Console.Write("\r{0}   ", s);
			}
		}

		private void OutputToDebug(string headerText = null)
		{
			if (!string.IsNullOrWhiteSpace(headerText))
			{
				Debug.WriteLine(headerText);
			}
			else
			{
				var s = BuildOutputString();
				Debug.Write("\r{0}   ", s);
			}
		}

		private string BuildOutputString()
		{
			StringBuilder sb = new StringBuilder();

			switch (_outputStyle)
			{
				case OutputStyle.CurrentOfMax:
					sb.Append(_current);
					sb.Append(" of ");
					sb.Append(_max);
					break;
				case OutputStyle.ASCIIBar:
					break;
			}

			return sb.ToString();
		}

		public void Dispose()
		{
			_stopWatch.Stop();
			_stopWatch = null;
		}

		public enum OutputLocation
		{
			Console,
			Debug
		}

		public enum OutputStyle
		{
			CurrentOfMax,
			ASCIIBar
		}
	}
}
