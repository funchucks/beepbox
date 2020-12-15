// Copyright (C) 2020 John Nesky, distributed under the MIT license.

import {Dictionary, DictionaryArray, EnvelopeType, InstrumentType, Transition, Chord, Envelope, Config} from "./SynthConfig";
import {NotePin, makeNotePin, Note, Pattern, Operator, SpectrumWave, HarmonicsWave, Instrument, Channel, Song, SynthBase} from "./SynthBase";

declare global {
	interface Window {
		AudioContext: any;
		webkitAudioContext: any;
	}
}

//namespace beepbox {

	export class Synth extends SynthBase {
		private audioCtx: any | null = null;
		private scriptNode: any | null = null;
		private liveInputEndTime: number = 0.0;
		
		constructor(song: Song | string | null = null) {
			super(song);
		}
		
		private activateAudio(): void {
			if (this.audioCtx == null || this.scriptNode == null) {
				this.audioCtx = this.audioCtx || new (window.AudioContext || window.webkitAudioContext)();
				this.samplesPerSecond = this.audioCtx.sampleRate;
				this.scriptNode = this.audioCtx.createScriptProcessor ? this.audioCtx.createScriptProcessor(2048, 0, 2) : this.audioCtx.createJavaScriptNode(2048, 0, 2); // 2048, 0 input channels, 2 output channels
				this.scriptNode.onaudioprocess = this.audioProcessCallback;
				this.scriptNode.channelCountMode = 'explicit';
				this.scriptNode.channelInterpretation = 'speakers';
				this.scriptNode.connect(this.audioCtx.destination);
			}
			this.audioCtx.resume();
		}
		
		private deactivateAudio(): void {
			if (this.audioCtx != null && this.scriptNode != null) {
				this.scriptNode.disconnect(this.audioCtx.destination);
				this.scriptNode = null;
				if (this.audioCtx.close) this.audioCtx.close(); // firefox is missing this function?
				this.audioCtx = null;
			}
		}
		
		public maintainLiveInput(): void {
			this.activateAudio();
			this.liveInputEndTime = performance.now() + 10000.0;
		}
		
		protected onSongEnd(): void {
			this.deactivateAudio();
		}
		
		public play(): void {
			super.play();
			this.activateAudio();
		}
		
		public stop(): void {
			this.pause();
			this.snapToStart();
			this.deactivateAudio();
		}
		
		private audioProcessCallback = (audioProcessingEvent: any): void => {
			const outputBuffer = audioProcessingEvent.outputBuffer;
			const outputDataL: Float32Array = outputBuffer.getChannelData(0);
			const outputDataR: Float32Array = outputBuffer.getChannelData(1);
			
			const isPlayingLiveTones = performance.now() < this.liveInputEndTime;
			if (!isPlayingLiveTones && !this.isPlayingSong) {
				for (let i: number = 0; i < outputBuffer.length; i++) {
					outputDataL[i] = 0.0;
					outputDataR[i] = 0.0;
				}
				this.deactivateAudio();
			} else {
				this.synthesize(outputDataL, outputDataR, outputBuffer.length, this.isPlayingSong);
			}
		}
		
	}
	
	// When compiling synth.ts as a standalone module named "beepbox", expose these classes as members to JavaScript:
	export {Dictionary, DictionaryArray, EnvelopeType, InstrumentType, Transition, Chord, Envelope, Config, NotePin, makeNotePin, Note, Pattern, Operator, SpectrumWave, HarmonicsWave, Instrument, Channel, Song};
//}
