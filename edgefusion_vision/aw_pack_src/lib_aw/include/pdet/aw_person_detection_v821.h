#ifndef AW_PERSON_DETECTION_H
#define AW_PERSON_DETECTION_H

#define	AW_AI_RESULT_NUM 20

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

#include "aw_image.h"

	typedef	struct _aw_detected_person AW_Det_Person;
	struct _aw_detected_person
	{
		AW_Box bbox;
		int label;
		float prob;
	};

	typedef struct _aw_detected_person_outputs AW_Det_Outputs;
	struct _aw_detected_person_outputs
	{
		int num;
		AW_Det_Person *persons;
	};

	/*
	* A struct that stores models, settings and history for the purpose of
	* person detection for an image.
	*
	* models:	  the handle of models.
	* settings:   the handle of runtime settings.
	* history:	  the handle of history information.
	*/
	typedef struct _aw_person_detection_container AW_Det_Container;
	struct _aw_person_detection_container
	{
		void *models;
		void *settings;
		void *history;
		void *options;
	};

	/*
	* A struct that stores params.
	*
	* target_size: the target image size.
	* threshold: the value for person label.
	*/
	typedef struct _aw_person_detection_settings AW_Det_Settings;
	struct _aw_person_detection_settings
	{
		int target_w;
		int target_h;
		int target_c;
		int target_size;
		float threshold;
	};

	/*
	* Initialization for container.
	*
	* Input:
	*   container:  the container need to be initialized.
	*   threshold:  the confidence threshold needs to be set when detecting.
	*               the threshold value range: [0.0, 1.0], default: 0.3.
	* Output:
	*	0,  initialized successfully.
	*  -1,  something wrong.
	*/
	int aw_init_person_det(AW_Det_Container *container, AW_Det_Outputs *outputs, int target_w, int target_h, int target_c, float threshold);

	/*
	* Run person detection.
	*
	* Input:
	*   container:    the initialized container.
	*   yuv_buf:	  the input yuv data which contains persons.
	*   threshold:    the confidence threshold needs to be set when detecting.
	*               the threshold value range: [0.0, 1.0], default: 0.3.
	* Output:
	*	label:		  result of person detection: .
	*	prob:		  result of person detection: .
	*   x,y,width,height: 	result of person detection: the rect
	*/
	void aw_run_person_det(AW_Det_Container *container, AW_Img *img, AW_Det_Outputs *outputs, float threshold);

	/*
	* Free memory for container and output.
	*/
	void aw_free_person_det(AW_Det_Container *container, AW_Det_Outputs *outputs);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // !AW_PERSON_DETECTION_H
