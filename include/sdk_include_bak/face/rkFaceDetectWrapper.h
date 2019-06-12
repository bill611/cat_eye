/*
 * Copyright (c) 2017 Rockchip Electronics Co. Ltd.
 * Author: Zeng Fei <felix.zeng@rock-chips.com>
 * Function: Face Detecting & Tracking
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __RK_FACE_DETECT_WRAPPER_H_
#define __RK_FACE_DETECT_WRAPPER_H_

#include <stdint.h>

#define RK_FACE_DETECT_VERSION "V7.1.0-2019-1-7"
#define MAX_FACE_COUNT 10


#define RK_FACE_DETECT_INPUT_WIDTH 320 /*must be a multiple of 16*/
#define RK_FACE_DETECT_INPUT_HEIGHT 240

#ifdef __cplusplus
extern "C" {
#endif

/* Object Information, rectangle and distance */
typedef struct Point {
    float x;
    float y;
} Point;

typedef struct rkFaceDetectInfo
{
    int x;
    int y;
    int width;
    int height;

    int id;
    float combined_score;//if face missed, combined_score=0
    Point landmarks[5];
    union {
		int detectedNum;
        int reserved[1];
    };

} rkFaceDetectInfo;

/* Object Information Array */
typedef struct rkFaceDetectInfos
{
    int index;
    int objectNum;
    int reserved[2]; // reserved for 32 bytes aligned
    rkFaceDetectInfo objects[MAX_FACE_COUNT];
} rkFaceDetectInfos;

/*
    Here generate the static context, and initialize about detecter everything.
    Suggest using default value: width x height = rkFaceDetectWrapper_getDefaultImageWidth() x rkFaceDetectWrapper_getDefaultImageHeight()
        width:
            Specifies the width of the detected image
        height:
            Specifies the height of the detected image
		
 */
void rkFaceDetectWrapper_initizlize(void *internal_virt, void *internal_phys, char* path_rk_face_detect_weight);

/*  The roi coordinate is relative to the original image */
int rkFaceDetectWrapper_detect(void *input_virt, void *input_phys, int width, int height, rkFaceDetectInfos *face_infos);

/*  Free everything */
void rkFaceDetectWrapper_release();


#ifdef __cplusplus
}
#endif

#endif // end of __RK_FACE_DETECT_WRAPPER_H_
