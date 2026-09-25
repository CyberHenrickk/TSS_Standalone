# TSS_Standalone
Includes the
# TSSAudioProcessor

TSSAudioProcessor is a small offline WAV audio processor created for the Totentanz Sound System Standalone mod for Cyberpunk 2077.

Its purpose is to prepare user-provided music files for playback through AudioXL. The tool applies a lightweight compressor/limiter to reduce clipping and distortion, then writes processed 16-bit PCM WAV files.

The program is executed manually by the user through the included build script. It is not part of the running game and does not modify Cyberpunk 2077.

## What the program does

TSSAudioProcessor:

1. Reads WAV files from the `#music` folder.
2. Decodes the samples into floating-point PCM data in memory.
3. Applies a compressor/limiter to control excessive peaks.
4. Writes the processed files as 16-bit PCM WAV files.
5. Saves the results to:

```text
audioxl_output/sounds/TotentanzSoundSystem/music/
