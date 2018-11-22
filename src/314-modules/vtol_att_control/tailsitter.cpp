/****************************************************************************
 *
 *   Copyright (c) 2015 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
* @file tailsitter.cpp
*
* @author Roman Bapst 		<bapstroman@gmail.com>
* @author David Vorsin     <davidvorsin@gmail.com>
*
*/
/* xj-zhang add first stage in trans*/
#include "tailsitter.h"
#include "vtol_att_control_main.h"

#define ARSP_YAW_CTRL_DISABLE 4.0f	// airspeed at which we stop controlling yaw during a front transition
#define THROTTLE_TRANSITION_MAX 0.1f	// maximum added thrust above last value in transition
#define PITCH_TRANSITION_FRONT_P1 -0.53f	 // pitch angle to switch to TRANSITION_P2,30 degrees
//#define GROUND_SPEED2_TRANSITION_FRONT_P1	16.0f // ground speed^2 to switch to TRANSITION_P2,5m/s*5m/s
#define PITCH_TRANSITION_FRONT_P2 -1.4f	// pitch angle to switch to FW,80 degrees
#define PITCH_TRANSITION_BACK_P1 -1.1f	// pitch angle to switch to MC,50 degrees
#define PITCH_TRANSITION_BACK_P2 -0.1f	// pitch angle to switch to MC
//xj-zhang
Tailsitter::Tailsitter(VtolAttitudeControl *attc) :
	VtolType(attc),
	_thrust_transition_start(0.0f),
	_yaw_transition(0.0f),
	_pitch_transition_start(0.0f)
{
	_vtol_schedule.flight_mode = MC_MODE;
	_vtol_schedule.transition_start = 0;

	_mc_roll_weight = 1.0f;
	_mc_pitch_weight = 1.0f;
	_mc_yaw_weight = 1.0f;

	_flag_was_in_trans_mode = false;
	//xj-zhang
	_params_handles_tailsitter.front_trans_dur_p2 = param_find("VT_TRANS_P2_DUR");
	_params_handles_tailsitter.trans_thr_min = param_find("VT_TRANS_THR_MIN");
	_params_handles_tailsitter.trans_thr_max = param_find("VT_TRANS_THR_MAX");
	_params_handles_tailsitter.GROUND_SPEED2_TRANSITION_FRONT_P1 = param_find("VT_TRAN_P1_GSPE");
}

void
Tailsitter::parameters_update()
{
	float v;

	/* vtol front transition phase 2 duration */
	//xj-zhang
	param_get(_params_handles_tailsitter.front_trans_dur_p2, &v);
	_params_tailsitter.front_trans_dur_p2 = v;
	param_get(_params_handles_tailsitter.trans_thr_min, &v);
	_params_tailsitter.trans_thr_min = v;
	param_get(_params_handles_tailsitter.trans_thr_max, &v);
	_params_tailsitter.trans_thr_max = v;
	param_get(_params_handles_tailsitter.GROUND_SPEED2_TRANSITION_FRONT_P1, &v);
	_params_tailsitter.GROUND_SPEED2_TRANSITION_FRONT_P1 = v;
}

void Tailsitter::update_vtol_state()
{

	/* simple logic using a two way switch to perform transitions.
	 * after flipping the switch the vehicle will start tilting in MC control mode, picking up
	 * forward speed. After the vehicle has picked up enough and sufficient pitch angle the uav will go into FW mode.
	 * For the backtransition the pitch is controlled in MC mode again and switches to full MC control reaching the sufficient pitch angle.
	*/

	matrix::Eulerf euler = matrix::Quatf(_v_att->q);
	float pitch = euler.theta();

	if (!_attc->is_fixed_wing_requested()) {

		switch (_vtol_schedule.flight_mode) { // user switchig to MC mode
		case MC_MODE:
			break;

		case FW_MODE:
			_vtol_schedule.flight_mode 	= TRANSITION_BACK_P1;
			_vtol_schedule.transition_start = hrt_absolute_time();
			break;

		case TRANSITION_FRONT_P1:
			// failsafe into multicopter mode
			_vtol_schedule.flight_mode = MC_MODE;
			break;
		case TRANSITION_FRONT_P2:
			// failsafe into multicopter mode
			_vtol_schedule.flight_mode = TRANSITION_BACK_P1;
			break;
		case TRANSITION_BACK_P1:
			// check if we have reached pitch angle to switch to MC mode
			if (pitch >= PITCH_TRANSITION_BACK_P1) {
				_vtol_schedule.flight_mode = TRANSITION_BACK_P2;
				_time_transition_start_p2=hrt_absolute_time();
			}

			break;
		case TRANSITION_BACK_P2:
			// check if we have reached pitch angle to switch to MC mode
			if (pitch >= PITCH_TRANSITION_BACK_P2) {
				_vtol_schedule.flight_mode = MC_MODE;
			}

			break;
		}

	} else {  // user switchig to FW mode

		switch (_vtol_schedule.flight_mode) {
		case MC_MODE:
			// initialise a front transition
			_vtol_schedule.flight_mode 	= TRANSITION_FRONT_P1;
			_vtol_schedule.transition_start = hrt_absolute_time();
			break;

		case FW_MODE:
			break;
			//xj-zhang
		case TRANSITION_FRONT_P1: {
			// check if we have reached ground speed  and pitch angle to switch to TRANSITION P2 mode
				float ground_speed_2=_local_pos->vx*_local_pos->vx+_local_pos->vy*_local_pos->vy;
				if ((ground_speed_2 >  _params_tailsitter.GROUND_SPEED2_TRANSITION_FRONT_P1 && pitch <= PITCH_TRANSITION_FRONT_P1) || can_transition_on_ground()) {
					_vtol_schedule.flight_mode = TRANSITION_FRONT_P2;
					_time_transition_start_p2=hrt_absolute_time();
				}
				break;
		}
		case TRANSITION_FRONT_P2: {

				bool airspeed_condition_satisfied = _airspeed->indicated_airspeed_m_s >= _params->transition_airspeed;
				airspeed_condition_satisfied |= _params->airspeed_disabled;

				// check if we have reached airspeed  and pitch angle to switch to TRANSITION P2 mode
				//xj-zhang
				if ((pitch <= PITCH_TRANSITION_FRONT_P2)) {
					_vtol_schedule.flight_mode = FW_MODE;
				}

				break;
			}

		case TRANSITION_BACK_P1:
			// failsafe into fixed wing mode
			_vtol_schedule.flight_mode = FW_MODE;
			break;
		case TRANSITION_BACK_P2:
			// failsafe into fixed wing mode
			//_vtol_schedule.flight_mode = FW_MODE;
			_vtol_schedule.flight_mode 	= TRANSITION_FRONT_P1;
			_vtol_schedule.transition_start = hrt_absolute_time();
			break;
		}
	}

	// map tailsitter specific control phases to simple control modes
	switch (_vtol_schedule.flight_mode) {
	case MC_MODE:
		_vtol_mode = ROTARY_WING;
		_vtol_vehicle_status->vtol_in_trans_mode = false;
		_flag_was_in_trans_mode = false;
		break;

	case FW_MODE:
		_vtol_mode = FIXED_WING;
		_vtol_vehicle_status->vtol_in_trans_mode = false;
		_flag_was_in_trans_mode = false;
		break;

	case TRANSITION_FRONT_P1:
		_vtol_mode = TRANSITION_TO_FW;
		_vtol_vehicle_status->vtol_in_trans_mode = true;
		_vtol_vehicle_status->vtol_trans_in_P1=true;
		break;
	case TRANSITION_FRONT_P2:
		_vtol_mode = TRANSITION_TO_FW;
		_vtol_vehicle_status->vtol_in_trans_mode = true;
		_vtol_vehicle_status->vtol_trans_in_P1=false;
		break;
	case TRANSITION_BACK_P1:
		_vtol_mode = TRANSITION_TO_MC;
		_vtol_vehicle_status->vtol_in_trans_mode = true;
		_vtol_vehicle_status->vtol_trans_in_P1=true;
		break;
	case TRANSITION_BACK_P2:
		_vtol_mode = TRANSITION_TO_MC;
		_vtol_vehicle_status->vtol_in_trans_mode = true;
		_vtol_vehicle_status->vtol_trans_in_P1=false;
		break;
	}
}

void Tailsitter::update_transition_state()
{
	float time_since_trans_start = (float)(hrt_absolute_time() - _vtol_schedule.transition_start) * 1e-6f;

	if (!_flag_was_in_trans_mode) {
		// save desired heading for transition and last thrust value
		_yaw_transition = _v_att_sp->yaw_body;
		//transition should start from current attitude instead of current setpoint
		matrix::Eulerf euler = matrix::Quatf(_v_att->q);
		_pitch_transition_start = euler.theta();
		_thrust_transition_start = _v_att_sp->thrust;
		_flag_was_in_trans_mode = true;
	}
	//xj-zhang
	if (_vtol_schedule.flight_mode == TRANSITION_FRONT_P1) {
		// in the first stage,set the pitch angle to the transition angle and keep it until the ground speed larger that the threshold
		_v_att_sp->pitch_body = _pitch_transition_start	- fabsf(PITCH_TRANSITION_FRONT_P1 - _pitch_transition_start) *
							time_since_trans_start/_params->front_trans_duration*4;
		_v_att_sp->pitch_body = math::constrain(_v_att_sp->pitch_body, PITCH_TRANSITION_FRONT_P1-0.1f,
									_pitch_transition_start);
		// disable mc yaw control once the plane has picked up speed
		if (_airspeed->indicated_airspeed_m_s > ARSP_YAW_CTRL_DISABLE) {
			_mc_yaw_weight = 0.0f;

		} else {
			_mc_yaw_weight = 1.0f;
		}
		_mc_roll_weight = _mc_pitch_weight=1.0f;
		matrix::Eulerf euler = matrix::Quatf(_v_att->q);
		_pitch_transition_start_p2= euler.theta();
		_v_att_sp->thrust = _mc_virtual_att_sp->thrust;
		_thrust_transition_start=_v_att_sp->thrust;
	}else if (_vtol_schedule.flight_mode == TRANSITION_FRONT_P2) {
		float scale=(hrt_absolute_time()-_time_transition_start_p2)*1e-6f/ (_params->front_trans_duration/2);
		float H=_params_tailsitter.trans_thr_max-_thrust_transition_start;

		// create time dependant pitch angle set point + 0.2 rad overlap over the switch value
		_v_att_sp->pitch_body = _pitch_transition_start_p2- fabsf(PITCH_TRANSITION_FRONT_P2 - _pitch_transition_start_p2) *scale;
		_v_att_sp->pitch_body = math::constrain(_v_att_sp->pitch_body, PITCH_TRANSITION_FRONT_P2-0.1f,
							_pitch_transition_start_p2);

		// disable mc yaw control once the plane has picked up speed
		_mc_yaw_weight = 0.0f;
		_mc_roll_weight = 0.0f;
		_mc_pitch_weight = 1.0f-scale;
		_v_att_sp->thrust =_thrust_transition_start+ math::constrain((H-H*(scale-1.0f)*(scale-1.0f)),0.0f,H);//THROTTLE_TRANSITION_MAX*scale;

	}else if(_vtol_schedule.flight_mode == TRANSITION_BACK_P1) {
		_v_att_sp->pitch_body =_pitch_transition_start+fabsf(PITCH_TRANSITION_BACK_P1-_pitch_transition_start)*
							time_since_trans_start / _params->back_trans_duration*2;
		_v_att_sp->pitch_body = math::constrain(_v_att_sp->pitch_body, -2.0f, PITCH_TRANSITION_BACK_P1 + 0.2f);
		_mc_yaw_weight = _mc_roll_weight=0.0f;
		_mc_pitch_weight=time_since_trans_start / _params->back_trans_duration*2;
		_v_att_sp->thrust=_thrust_transition_start;
		matrix::Eulerf euler = matrix::Quatf(_v_att->q);
		_pitch_transition_start_p2= euler.theta();
	}
	else if (_vtol_schedule.flight_mode == TRANSITION_BACK_P2) {
		float scale=(hrt_absolute_time()-_time_transition_start_p2)*1e-6f/ _params->back_trans_duration*6;
		if (!flag_idle_mc) {
			flag_idle_mc = set_idle_mc();
		}

		// create time dependant pitch angle set point stating at -pi/2 + 0.2 rad overlap over the switch value
		//xj-zhang俯仰角设定值应当在垂直坐标系中给出，直接设置到0附近过于暴力
		//_v_att_sp->pitch_body = M_PI_2_F + _pitch_transition_start + fabsf(PITCH_TRANSITION_BACK + 1.57f) *
		_v_att_sp->pitch_body =_pitch_transition_start_p2+fabsf(PITCH_TRANSITION_BACK_P2-_pitch_transition_start_p2)*scale;
		_v_att_sp->pitch_body = math::constrain(_v_att_sp->pitch_body, PITCH_TRANSITION_BACK_P1, PITCH_TRANSITION_BACK_P2 + 0.1f);

		// keep yaw disabled
		_mc_yaw_weight = 0.0f;

		// smoothly move control weight to MC
		_mc_roll_weight = scale;
		_mc_pitch_weight=1;
		//xj-zhang test
		_v_att_sp->thrust=_mc_virtual_att_sp->thrust;//_thrust_transition_start;
	}
	//xj-zhang
	_v_att_sp->thrust = math::constrain(_v_att_sp->thrust,_params_tailsitter.trans_thr_min,0.9f);
	_mc_roll_weight = math::constrain(_mc_roll_weight, 0.0f, 1.0f);
	_mc_yaw_weight = math::constrain(_mc_yaw_weight, 0.0f, 1.0f);
	_mc_pitch_weight = math::constrain(_mc_pitch_weight, 0.0f, 1.0f);

	// compute desired attitude and thrust setpoint for the transition

	_v_att_sp->timestamp = hrt_absolute_time();
	_v_att_sp->roll_body = 0.0f;
	_v_att_sp->yaw_body = _yaw_transition;

	matrix::Quatf q_sp = matrix::Eulerf(_v_att_sp->roll_body, _v_att_sp->pitch_body, _v_att_sp->yaw_body);
	q_sp.copyTo(_v_att_sp->q_d);
	_v_att_sp->q_d_valid = true;
}

void Tailsitter::waiting_on_tecs()
{
	// copy the last trust value from the front transition
	_v_att_sp->thrust = _thrust_transition;
}

void Tailsitter::update_mc_state()
{
	VtolType::update_mc_state();
}

void Tailsitter::update_fw_state()
{
	VtolType::update_fw_state();
}

/**
* Write data to actuator output topic.
*/
void Tailsitter::fill_actuator_outputs()
{
	_actuators_out_0->timestamp = hrt_absolute_time();
	_actuators_out_0->timestamp_sample = _actuators_mc_in->timestamp_sample;

	_actuators_out_1->timestamp = hrt_absolute_time();
	_actuators_out_1->timestamp_sample = _actuators_fw_in->timestamp_sample;

	switch (_vtol_mode) {
	case ROTARY_WING:
		_actuators_out_0->control[actuator_controls_s::INDEX_ROLL] = _actuators_mc_in->control[actuator_controls_s::INDEX_ROLL];
		_actuators_out_0->control[actuator_controls_s::INDEX_PITCH] =
			_actuators_mc_in->control[actuator_controls_s::INDEX_PITCH];
		_actuators_out_0->control[actuator_controls_s::INDEX_YAW] = _actuators_mc_in->control[actuator_controls_s::INDEX_YAW];
		_actuators_out_0->control[actuator_controls_s::INDEX_THROTTLE] =
			_actuators_mc_in->control[actuator_controls_s::INDEX_THROTTLE];

		if (_params->elevons_mc_lock) {
			_actuators_out_1->control[0] = 0;
			_actuators_out_1->control[1] = 0;

		} else {
			// NOTE: There is no mistake in the line below, multicopter yaw axis is controlled by elevon roll actuation!
			_actuators_out_1->control[actuator_controls_s::INDEX_ROLL] =
				_actuators_mc_in->control[actuator_controls_s::INDEX_YAW];	//roll elevon
			_actuators_out_1->control[actuator_controls_s::INDEX_PITCH] =
				_actuators_mc_in->control[actuator_controls_s::INDEX_PITCH];	//pitch elevon
		}

		break;

	case FIXED_WING:
		// in fixed wing mode we use engines only for providing thrust, no moments are generated
		_actuators_out_0->control[actuator_controls_s::INDEX_ROLL] =  _actuators_fw_in->control[actuator_controls_s::INDEX_YAW];	// yaw
		_actuators_out_0->control[actuator_controls_s::INDEX_PITCH] = 0;//_actuators_fw_in->control[actuator_controls_s::INDEX_PITCH]*1.0f;
		_actuators_out_0->control[actuator_controls_s::INDEX_YAW] = 0;
		_actuators_out_0->control[actuator_controls_s::INDEX_THROTTLE] =
			_actuators_fw_in->control[actuator_controls_s::INDEX_THROTTLE];

		_actuators_out_1->control[actuator_controls_s::INDEX_ROLL] =
			-_actuators_fw_in->control[actuator_controls_s::INDEX_ROLL];	// roll elevon
		_actuators_out_1->control[actuator_controls_s::INDEX_PITCH] =
			_actuators_fw_in->control[actuator_controls_s::INDEX_PITCH];	// pitch elevon
		_actuators_out_1->control[actuator_controls_s::INDEX_YAW] =
			_actuators_fw_in->control[actuator_controls_s::INDEX_YAW];	// yaw
		_actuators_out_1->control[actuator_controls_s::INDEX_THROTTLE] =
			_actuators_fw_in->control[actuator_controls_s::INDEX_THROTTLE];	// throttle
		break;

	case TRANSITION_TO_FW:
	case TRANSITION_TO_MC:
		// in transition engines are mixed by weight (BACK TRANSITION ONLY)
		_actuators_out_0->control[actuator_controls_s::INDEX_ROLL] = _actuators_mc_in->control[actuator_controls_s::INDEX_ROLL]* _mc_roll_weight;
		_actuators_out_0->control[actuator_controls_s::INDEX_PITCH] =_actuators_mc_in->control[actuator_controls_s::INDEX_PITCH] * _mc_pitch_weight;
		_actuators_out_0->control[actuator_controls_s::INDEX_YAW] = _actuators_mc_in->control[actuator_controls_s::INDEX_YAW] *_mc_yaw_weight;
		_actuators_out_0->control[actuator_controls_s::INDEX_THROTTLE] =
			_actuators_mc_in->control[actuator_controls_s::INDEX_THROTTLE];

		// NOTE: There is no mistake in the line below, multicopter yaw axis is controlled by elevon roll actuation!
		_actuators_out_1->control[actuator_controls_s::INDEX_ROLL] =//_actuators_mc_in->control[actuator_controls_s::INDEX_YAW]*_mc_yaw_weight+
		-_actuators_fw_in->control[actuator_controls_s::INDEX_ROLL]* (1 - _mc_yaw_weight);
		_actuators_out_1->control[actuator_controls_s::INDEX_PITCH] =
			//_actuators_mc_in->control[actuator_controls_s::INDEX_PITCH] * _mc_pitch_weight+
			_actuators_fw_in->control[actuator_controls_s::INDEX_PITCH] *(1 - _mc_pitch_weight);
		 //(_actuators_fw_in->control[actuator_controls_s::INDEX_PITCH] + _params->fw_pitch_trim) *(1 - _mc_pitch_weight);
		_actuators_out_1->control[actuator_controls_s::INDEX_THROTTLE] =
			_actuators_fw_in->control[actuator_controls_s::INDEX_THROTTLE];
		break;
	}
}
