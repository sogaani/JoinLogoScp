//
// logoframe by Yobi
//
//	AvisynthがGPLなので、このソフトもGPLにします。
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#else
#include <strings.h>
#endif
#include <fcntl.h>
#include "avs_internal.c"
#include "logo.h"
#include "logoset.h"
#include "logoset_mul.h"

#ifndef _WIN32
#define stricmp strcasecmp
typedef unsigned char BYTE;
#endif

// logo function
extern void MultLogoInit(MLOGO_DATASET* pml);
extern void MultLogoFree(MLOGO_DATASET* pml);
extern int  MultLogoOptionAdd(MLOGO_DATASET* pml, const char* strcmd, const char* strval);
extern int  MultLogoOptionFile(MLOGO_DATASET* pml, const char* fname);
extern int  MultLogoOptionOrgFile(MLOGO_DATASET* pml);
extern int  MultLogoSetup(MLOGO_DATASET* pml, int num_frames);
extern void MultLogoDisplayParam(MLOGO_DATASET* pml);
extern void MultLogoCalc(MLOGO_DATASET* pml, const BYTE *data, int pitch, int nframe, int height);
extern void MultLogoFind(MLOGO_DATASET* pml);
extern int  MultLogoWrite(MLOGO_DATASET* pml);


#ifndef INT_MAX
#define INT_MAX 0x7fffffff
#endif

#define MY_VERSION "logoframe 1.20"
#define AVS_BUFSIZE 128*1024
//#define FILE_BUFSIZE 512
#define CSP_I420 1
#define CSP_I422 2
#define CSP_I444 3

static int csp_to_int(const char *arg)
{
    if(!strcasecmp(arg, "i420"))
        return CSP_I420;
    if(!strcasecmp(arg, "i422"))
        return CSP_I422;
    if(!strcasecmp(arg, "i444"))
        return CSP_I444;
    return 0;
}


int main(int argc, const char* argv[])
{
    const char* infile   = NULL;
    int err_count = 0;
    int errforce = 0;
    int nopt_fromini = 0;
    int nopt_fromfile = 0;
    int nodisp = 0;
    int usage = 0;
    int seek = 0;
    int end = 0;
    int interlaced = 0;
    int tff = 0;
    int csp = CSP_I420;
    // logo data
    MLOGO_DATASET logodata;     // for LOGO


	//--- initialize ---
	MultLogoInit( &logodata );  // for LOGO

	//--- read options from .ini file ---
	nopt_fromini = MultLogoOptionOrgFile( &logodata );  // for LOGO
	if (nopt_fromini < 0){
		nopt_fromini = 0;
	}
	//--- read options from file by "-F" ---
    for(int i = 1; i < argc; i++) {
		if (!stricmp(argv[i], "-F")){
			if(i > argc-2) {
				fprintf(stderr, "-F needs an argument\n");
				return 2;
			}
			int nopt = MultLogoOptionFile( &logodata, argv[++i] );  // for LOGO
			if (nopt >= 0){
				nopt_fromfile += nopt;
			}
			else{
				fprintf(stderr, "warning:not found '%s'\n", argv[i]);
				err_count ++;
			}
		}
	}
	//--- options ---
    for(int i = 1; i < argc; i++) {
        if(argv[i][0] == '-' && argv[i][1] != 0) {
			const char* strval;
			int   ncnt;

			if (i > argc-2){
				strval = NULL;
			}
			else{
				strval = argv[i+1];
			}
			ncnt = MultLogoOptionAdd( &logodata, argv[i], strval );  // for LOGO
			if (ncnt > 0){
				i += (ncnt - 1);
			} else if (ncnt < 0){
				return 2;
            } else if (!stricmp(argv[i], "-F")) {
				i ++;
            } else if (!stricmp(argv[i], "-nodisp")) {
                nodisp = 0;
            } else if(!stricmp(argv[i], "-errforce")) {
                errforce = 1;
            } else if (!stricmp(argv[i], "-h")) {
                usage = 1;
            } else if(!stricmp(argv[i], "-seek")) {
                if(i > argc-2) {
                    fprintf(stderr, "-seek needs an argument\n");
                    return 2;
                }
                seek = atoi(argv[++i]);
                if(seek < 0) usage = 1;
            } else if(!stricmp(argv[i], "-frames")) {
                if(i > argc-2) {
                    fprintf(stderr, "-frames needs an argument\n");
                    return 2;
                }
                end = atoi(argv[++i]);
            } else if(!stricmp(argv[i], "-csp")) {
                if(i > argc-2) {
                    fprintf(stderr, "-csp needs an argument\n");
                    return 2;
                }
                csp = csp_to_int(argv[++i]);
                if(!csp) {
                    fprintf(stderr, "-csp '%s' is unknown\n", argv[i]);
                    return 2;
                }
            } else {
                fprintf(stderr, "Warning: no such option: %s\n", argv[i]);
                err_count ++;
                if (i <= argc-2){
                    const char *dot = strrchr(argv[i+1], '.');
                    if (!dot){
                        dot = "";
                    }
                    if (argv[i+1][0] != '-' && strcmp(".avs", dot)){
                        i ++;
                    }
                }
            }
        } else if(!infile) {
            infile = argv[i];
            const char *dot = strrchr(infile, '.');
            if (!dot){
                dot = "";
            }
            if(strcmp(".avs", dot))
                fprintf(stderr, "infile '%s' doesn't look like an avisynth script\n", infile);
        } else {
            fprintf(stderr, "too many input files %s\n", argv[i]);
            return 2;
        }
    }

    if(usage || !infile){
        fprintf(stderr, MY_VERSION "\n"
        "Usage: logoframe [options] in.avs [-logo data.lgd] [-oa outa.txt] [-o out.avs]\n"
		);
		return 2;
	}
	if (err_count > 0 && errforce > 0){
        fprintf(stderr, MY_VERSION "\n"
        "Forced Exit (error found in arguments)\n"
		);
		return 2;
	}
	if (logodata.dispoff > 0){  // for LOGO
		nodisp = 1;
	}
	if (nopt_fromini > 0 && nodisp == 0){
		printf("Info: read %d options from logoframe.ini\n", nopt_fromini);
	}
	if (nopt_fromfile > 0 && nodisp == 0){
		printf("Info: read %d options from -F file\n", nopt_fromfile);
	}


	//--- avisynth ---
    int retval = 1;
    avs_hnd_t avs_h = {0};
    if(internal_avs_load_library(&avs_h) < 0) {
        fprintf(stderr, "error: failed to load avisynth.dll\n");
        goto fail;
    }

    avs_h.env = avs_h.func.avs_create_script_environment(AVS_INTERFACE_25);
    if(avs_h.func.avs_get_error) {
        const char *error = avs_h.func.avs_get_error(avs_h.env);
        if(error) {
            fprintf(stderr, "error: %s\n", error);
            goto fail;
        }
    }

    AVS_Value arg = avs_new_value_string(infile);
    AVS_Value res = avs_h.func.avs_invoke(avs_h.env, "Import", arg, NULL);
    if(avs_is_error(res)) {
        fprintf(stderr, "error: %s\n", avs_as_string(res));
        goto fail;
    }
    /* check if the user is using a multi-threaded script and apply distributor if necessary.
       adapted from avisynth's vfw interface */
    AVS_Value mt_test = avs_h.func.avs_invoke(avs_h.env, "GetMTMode", avs_new_value_bool(0), NULL);
    int mt_mode = avs_is_int(mt_test) ? avs_as_int(mt_test) : 0;
    avs_h.func.avs_release_value(mt_test);
    if( mt_mode > 0 && mt_mode < 5 ) {
        AVS_Value temp = avs_h.func.avs_invoke(avs_h.env, "Distributor", res, NULL);
        avs_h.func.avs_release_value(res);
        res = temp;
    }
    if(!avs_is_clip(res)) {
        fprintf(stderr, "error: '%s' didn't return a video clip\n", infile);
        goto fail;
    }
    avs_h.clip = avs_h.func.avs_take_clip(res, avs_h.env);
    const AVS_VideoInfo *inf = avs_h.func.avs_get_video_info(avs_h.clip);
    if(!avs_has_video(inf)) {
        fprintf(stderr, "error: '%s' has no video data\n", infile);
        goto fail;
    }
    /* if the clip is made of fields instead of frames, call weave to make them frames */
    if(avs_is_field_based(inf)) {
        fprintf(stderr, "detected fieldbased (separated) input, weaving to frames\n");
        AVS_Value tmp = avs_h.func.avs_invoke(avs_h.env, "Weave", res, NULL);
        if(avs_is_error(tmp)) {
            fprintf(stderr, "error: couldn't weave fields into frames\n");
            goto fail;
        }
        res = internal_avs_update_clip(&avs_h, &inf, tmp, res);
        interlaced = 1;
        tff = avs_is_tff(inf);
    }
    fprintf(stderr, "%s: %dx%d, ", infile, inf->width, inf->height);
    if(inf->fps_denominator == 1)
        fprintf(stderr, "%d fps, ", inf->fps_numerator);
    else
        fprintf(stderr, "%d/%d fps, ", inf->fps_numerator, inf->fps_denominator);
    fprintf(stderr, "%d frames\n", inf->num_frames);

    if( (csp == CSP_I420 && 12 != avs_bits_per_pixel(inf)) ||
        (csp == CSP_I422 && 16 != avs_bits_per_pixel(inf)) ||
        (csp == CSP_I444 && 24 != avs_bits_per_pixel(inf)) )
    {
        const char *csp_name = csp == CSP_I444 ? "YV24" :
                               csp == CSP_I422 ? "YV16" :
                               "YV12";
        fprintf(stderr, "converting input clip to %s\n", csp_name);
        if(csp < CSP_I444 && (inf->width&1)) {
            fprintf(stderr, "error: input clip width not divisible by 2 (%dx%d)\n", inf->width, inf->height);
            goto fail;
        }
        if(csp == CSP_I420 && interlaced && (inf->height&3)) {
            fprintf(stderr, "error: input clip height not divisible by 4 (%dx%d)\n", inf->width, inf->height);
            goto fail;
        }
        if((csp == CSP_I420 || interlaced) && (inf->height&1)) {
            fprintf(stderr, "error: input clip height not divisible by 2 (%dx%d)\n", inf->width, inf->height);
            goto fail;
        }
        const char *arg_name[2] = {NULL, "interlaced"};
        AVS_Value arg_arr[2] = {res, avs_new_value_bool(interlaced)};
        char conv_func[14] = {"ConvertTo"};
        strcat(conv_func, csp_name);
        AVS_Value tmp = avs_h.func.avs_invoke(avs_h.env, conv_func, avs_new_value_array(arg_arr, 2), arg_name);
        if(avs_is_error(tmp)) {
            fprintf(stderr, "error: couldn't convert input clip to %s\n", csp_name);
            goto fail;
        }
        res = internal_avs_update_clip(&avs_h, &inf, tmp, res);
    }
    avs_h.func.avs_release_value(res);


    char *interlace_type = interlaced ? tff ? "t" : "b" : "p";
    char *csp_type = NULL;
//    int chroma_h_shift = 0;
//    int chroma_v_shift = 0;
    switch(csp) {
        case CSP_I420:
            csp_type = "420mpeg2";
//            chroma_h_shift = 1;
//            chroma_v_shift = 1;
            break;
        case CSP_I422:
            csp_type = "422";
//            chroma_h_shift = 1;
            break;
        case CSP_I444:
            csp_type = "444";
            break;
        default:
            goto fail; //can't happen
    }
    if (nodisp == 0){
	    printf("YUV4MPEG2 W%d H%d F%u:%u I%s A0:0 C%s\n",
	        inf->width, inf->height, inf->fps_numerator, inf->fps_denominator, interlace_type, csp_type);
	}

    end += seek;
    if(end <= seek || end > inf->num_frames)
        end = inf->num_frames;


//--------------- start logoframe from here ---------------
	int errnum;
	int height;

	// for checking if logodata area is out of image data
	height = inf->height;

	// setup for logo detection
	errnum = MultLogoSetup( &logodata, inf->num_frames );  // for LOGO
	if (errnum != 0){
		if (errnum == 3){		// no logo definition found
			retval = 0;
		}
		goto fail;
	}

	// display parameter
	if (nodisp == 0 && logodata.paramoff == 0){   // for LOGO
		MultLogoDisplayParam( &logodata );        // for LOGO
	}
	if (nodisp == 0){
		printf("Total logodata : %d\n", logodata.num_deflogo);  // for LOGO
	}

	// read framedata
    if (nodisp == 0){
		printf("checking %6d/%d start.\n", seek, end-1);
	}
    for(int frm = seek; frm < end; ++frm) {
        //--- avisynth start ---
        AVS_VideoFrame *f = avs_h.func.avs_get_frame(avs_h.clip, frm);
        const char *err = avs_h.func.avs_clip_get_error(avs_h.clip);
        if(err) {
            fprintf(stderr, "error: %s occurred while reading frame %d\n", err, frm);
            goto fail;
        }
        static const int planes[] = {AVS_PLANAR_Y, AVS_PLANAR_U, AVS_PLANAR_V};
        int pitch = avs_get_pitch_p(f, planes[0]);
        const BYTE* data = avs_get_read_ptr_p(f, planes[0]);
        //--- avisynth end ---

		// detect logo in a picture
		MultLogoCalc( &logodata, data, pitch, frm, height );    // for LOGO

		if (nodisp == 0){
			if (((frm % 5000 == 0) && (frm != 0)) ||
				(frm == 100) ||
				(frm == 1000)){
				printf("checking %6d/%d ended.\n", frm, end-1);
			}
		}

		//--- avisynth command ---
        avs_h.func.avs_release_video_frame(f);
	}
	if (nodisp == 0){
		printf("checking %6d/%d ended.\n", end-1, end-1);
	}

	// find logo-on frames
	MultLogoFind( &logodata );     // for LOGO

	// output result
	MultLogoWrite( &logodata );    // for LOGO


//--------------- end logoframe ---------------


//close_files:
    retval = 0;
fail:
	MultLogoFree( &logodata );     // for LOGO

	//--- avisynth command ---
    if(avs_h.library)
        internal_avs_close_library(&avs_h);
	return retval;
}
