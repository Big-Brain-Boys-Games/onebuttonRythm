#ifndef MAINC
#include "files.h"
#endif

Map loadMapInfo(char * file)
{
	Map map;
	map.folder = malloc(100);
	strcpy(map.folder, file);
	char * pStr = malloc(strlen(file) + 12);
	strcpy(pStr, file);
	strcat(pStr, "/image.png");
	map.image = LoadTexture(pStr);
	
	strcpy(pStr, file);
	strcat(pStr, "/map.data");
	FILE * f;
	f = fopen(pStr, "rb");

	//text file
	char line [1000];
	enum FilePart mode = fpNone;
	while(fgets(line,sizeof(line),f)!= NULL)
	{
		int stringLenght = strlen(line);
		bool emptyLine = true;
		for(int i = 0; i < stringLenght; i++)
			if(line[i] != ' ' && line[i] != '\n')
				emptyLine = false;
		
		if(emptyLine)
			continue;

		if(strcmp(line, "[Name]\n") == 0)			{mode = fpName;			continue;}
		if(strcmp(line, "[Creator]\n") == 0)		{mode = fpCreator;		continue;}
		if(strcmp(line, "[Difficulty]\n") == 0)		{mode = fpDifficulty;	continue;}
		if(strcmp(line, "[BPM]\n") == 0)			{mode = fpBPM;	continue;}
		if(strcmp(line, "[Notes]\n") == 0)			{mode = fpNotes;		continue;}
		printf("%i mode, %s", mode, line);
		for(int i = 0; i < 100; i++)
					if(line[i] == '\n') line[i]= '\0';
		switch(mode)
		{
			case fpNone:
				break;
			case fpName:
				map.name = malloc(100);
				strcpy(map.name, line);
				break;
			case fpCreator:
				map.creator = malloc(100);
				strcpy(map.creator, line);
				//todo save creator
				break;
			case fpDifficulty:
				map.difficulty = atoi(line);
				//todo save difficulty
				break;
			case fpBPM:
				map.bpm = atoi(line);
				break;
			case fpNotes:
				//neat, notes :P
				break;
		}
	}
	fclose(f);
	free(pStr);

	if(map.image.id == 0)
	{
		printf("no background texture found\n");
		map.image = LoadTexture("background.png");
		// _noBackground = 1;
	}
	return map;
}

void saveFile (int noteAmount)
{
	printf("written map data\n");
	char * mapName = "Map1";
	char * Creator = "Me";
	int Difficulty = 3;
	fprintf(_pFile, "[Name]\n");
	fprintf(_pFile, "%s\n", mapName);
	fprintf(_pFile, "[Creator]\n");
	fprintf(_pFile, "%s\n", Creator);
	fprintf(_pFile, "[Difficulty]\n");
	fprintf(_pFile, "%i\n", Difficulty);
	fprintf(_pFile, "[BPM]\n");
	fprintf(_pFile, "%i\n", bpm);
	fprintf(_pFile, "[Notes]\n");
	for(int i = 0; i < noteAmount; i++)
	{
		if(_pNotes[i] == 0)
			continue;
		fprintf(_pFile, "%f\n", _pNotes[i]);
	}
	fclose(_pFile);
	//fwrite(_pNotes, sizeof(float), noteIndex, _pFile);
	
}

void * loadAudio(char * file, ma_decoder * decoder, int * audioLength)
{
	ma_result result;
	printf("loading sound effect %s\n", file);
	ma_decoder_config decoder_config = ma_decoder_config_init(ma_format_f32, 2, 48000);
	decoder_config.resampling.linear.lpfOrder = MA_MAX_FILTER_ORDER;
	result = ma_decoder_init_file(file, &decoder_config, decoder);
    if (result != MA_SUCCESS) {
        printf("failed to open music file %s\n", file);
		exit(0);
    }
	printf("ma_resample_algorithm %i\n", decoder_config.resampling.algorithm);
	printf("decoder format %i   Sizeof %i\n", decoder->outputFormat, sizeof(_Float32));
	int lastFrame = -1;
	ma_decoder_get_length_in_pcm_frames(decoder, audioLength);
	printf("audio length %i\n", *audioLength);
	printf("audio samplerate: %i\n", decoder->outputSampleRate);
	void * pAudio = calloc(sizeof(_Float32)*2*2, *audioLength); //added some patting to get around memory issue //todo fix this work around
	void * pCursor = pAudio;
	printf("doing resampling %i\n", decoder->converter.hasResampler);
	while(decoder->readPointerInPCMFrames !=lastFrame)
	{
		lastFrame = decoder->readPointerInPCMFrames;
		ma_decoder_read_pcm_frames(decoder, pCursor, 256, NULL);
		pCursor += sizeof(_Float32)*2*256;
	}
	ma_decoder_uninit(decoder);
	return pAudio;
}

void loadMusic(char * file)
{
	_musicPlaying = false;
	if(_musicLength != 0)
	{
		//unload previous music
		free(_pMusic);
	}

	_pMusic = loadAudio(file, &_decoder, &_musicLength);

	printf("yoo %i\n", _decoder.outputSampleRate);

	_deviceConfig = ma_device_config_init(ma_device_type_playback);
    _deviceConfig.playback.format   = _decoder.outputFormat;
    _deviceConfig.playback.channels = _decoder.outputChannels;
    _deviceConfig.sampleRate        = _decoder.outputSampleRate;
    _deviceConfig.dataCallback      = data_callback;
    _deviceConfig.pUserData         = &_decoder;
	// _deviceConfig.periodSizeInMilliseconds = 300;
	
}

void loadMap (int fileType)
{
	char * pStr = malloc(strlen(_pMap) + 12);
	strcpy(pStr, _pMap);
	strcat(pStr, "/image.png");
	_background = LoadTexture(pStr);
	strcpy(pStr, _pMap);
	strcat(pStr, "/song.mp3");

	// ma_result result
	// music = LoadMusicStream(pStr);
	loadMusic(pStr);
	

	strcpy(pStr, _pMap);
	strcat(pStr, "/map.data");
	if(fileType == 0)
	{
		_pFile = fopen(pStr, "rb");

		//text file
		char line [1000];
		enum FilePart mode = fpNone;
		_amountNotes = 50;
		_noteIndex = 0;
		_pNotes = malloc(sizeof(float)*_amountNotes);
   		while(fgets(line,sizeof(line),_pFile)!= NULL)
		{
			int stringLenght = strlen(line);
			bool emptyLine = true;
			for(int i = 0; i < stringLenght; i++)
				if(line[i] != ' ' && line[i] != '\n')
					emptyLine = false;
			
			if(emptyLine)
				continue;

			if(strcmp(line, "[Name]\n") == 0)			{mode = fpName;			continue;}
			if(strcmp(line, "[Creator]\n") == 0)		{mode = fpCreator;		continue;}
			if(strcmp(line, "[Difficulty]\n") == 0)		{mode = fpDifficulty;	continue;}
			if(strcmp(line, "[BPM]\n") == 0)			{mode = fpBPM;			continue;}
			if(strcmp(line, "[Notes]\n") == 0)			{mode = fpNotes;		continue;}
			printf("%i mode, %s", mode, line);
			switch(mode)
			{
				case fpNone:
					break;
				case fpName:
					//todo save name
					break;
				case fpCreator:
					//todo save creator
					break;
				case fpDifficulty:
					//todo save difficulty
					break;
				case fpBPM:
					bpm = atoi(line);
					printf("set bpm: %i\n", bpm);	
					break;
				case fpNotes:
					
					if(_noteIndex <= _amountNotes)
					{
						_amountNotes += 50;
						_pNotes = realloc(_pNotes, _amountNotes);
					}
					_pNotes[_noteIndex] = atof(line);
					printf("note %i  %f\n", _noteIndex, _pNotes[_noteIndex]);
					_noteIndex++;
					break;
			}
		}
		_amountNotes = _noteIndex;
		_noteIndex = 0;
		fclose(_pFile);
	}else
	{
		_pFile = fopen(pStr, "wb");
	}
	free(pStr);

	if(_background.id == 0)
	{
		printf("no background texture found\n");
		_background = LoadTexture("background.png");
		_noBackground = 1;
	}
}

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{

	// music
	if (_musicPlaying)
	{
		if (_musicLength > _musicFrameCount)
			memcpy(pOutput, _pMusic + _musicFrameCount * sizeof(_Float32) * 2, frameCount * sizeof(_Float32) * 2);
		_musicFrameCount += frameCount;
	}
	// sound effects

	for (int i = 0; i < frameCount * 2; i++)
	{
		((_Float32 *)pOutput)[i] += ((_Float32 *)_pEffectsBuffer)[(i + _effectOffset) % (48000 * 4)];
		// printf("i %i \t actual position %i \t %.1f\n", i, i + _effectOffset, ((_Float32*)_pEffectsBuffer)[(i+_effectOffset)%(4800*4)]);
		((_Float32 *)_pEffectsBuffer)[(i + _effectOffset) % (48000 * 4)] = 0;
	}
	_effectOffset += frameCount * 2;

	(void)pInput;
}

void saveScore()
{
	FILE * file;
	char str [100];
	strcpy(str, "scores/");
	strcat(str, _pMap);
	if(!DirectoryExists("scores/"))
		return;
	file = fopen(str, "w");
	//todo add combo
	fprintf(file, "%i %i\n", _score, 0);
	fclose(file);
}

bool readScore(char * map, int *score, int * combo)
{
	score = 0;
	combo = 0;
	FILE * file;
	char str [100];
	strcpy(str, "scores/");
	strcat(str, _pMap);
	if(!DirectoryExists("scores/"))
		return;
	if(!FileExists(str))
		return;
	file = fopen(str, "r");
	//todo add combo
	char line [1000];
	while(fgets(line,sizeof(line),_pFile)!= NULL)
	{
		_score = atoi(line);
		char * part = &line[0];
		for(int i = 0; i < 1000; i++)
		{
			if(*part==' ');
				break;
			part++;
		}
		//combo = atoi(part);
	}
	fclose(file);
}
