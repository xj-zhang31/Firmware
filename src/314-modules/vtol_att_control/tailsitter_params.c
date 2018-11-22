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
 * @file tailsitter_params.c
 * Parameters for vtol attitude controller.
 *
 * @author Roman Bapst <bapstroman@gmail.com>
 * @author David Vorsin     <davidvorsin@gmail.com>
 */

/**
 * Duration of front transition phase 2
 *
 * Time in seconds it should take for the rotors to rotate forward completely from the point
 * when the plane has picked up enough airspeed and is ready to go into fixed wind mode.
 *
 * @unit s
 * @min 0.1
 * @max 5.0
 * @increment 0.01
 * @decimal 3
 * @group VTOL Attitude Control

PARAM_DEFINE_FLOAT(VT_TRANS_P2_DUR, 0.5f);*/
/**
 * The channel number of motors that must be turned off in fixed wing mode.
 *
 * @min 0
 * @max 12345678
 * @increment 1
 * @decimal 0
 * @group VTOL Attitude Control
 */
PARAM_DEFINE_INT32(VT_FW_MOT_OFFID, 0);
/**
 * The min thrust setpoint in transition ,tailsitter
 *
 * @min 0
 * @max 1.0
 * @increment 0.1
 * @decimal 1
 * @group VTOL Attitude Control
 */
PARAM_DEFINE_FLOAT(ZXJ_TRAN_THR_MIN, 0.4);
/**
 * The max thrust setpoint in transition ,tailsitter
 *
 * @min 0
 * @max 1.0
 * @increment 0.1
 * @decimal 1
 * @group VTOL Attitude Control
 */
PARAM_DEFINE_FLOAT(ZXJ_TRAN_THR_MAX, 0.8);
/**
 * Fixed wing pitch trim
 *
 * This parameter allows to adjust the neutral elevon position in fixed wing mode.
 *
 * @min -1.0
 * @max 1.0
 * @increment 0.01
 * @decimal 2
 * @group VTOL Attitude Control
 */
PARAM_DEFINE_FLOAT(ZXJ_FWPITCH_TRIM, 0.0f);
/**
 * turn off small motors
 *
 * If set to 1 , pilot can use  flag to contorl small rotors
 *
 * @boolean
 * @group VTOL Attitude Control
 */
PARAM_DEFINE_INT32(ZXJ_MOTOFF_TEST, 0);
/**
 * transition condition -- ground speed^2 (m/s)^2
 *
 * This parameter is the condition for tailsitter translate speed from forward P1 to P2.
 *
 * @min 0
 * @max 100.0
 * @increment 0.01
 * @decimal 2
 * @group VTOL Attitude Control
 */
PARAM_DEFINE_FLOAT(ZXJ_TRANP1_GSPE, 16.0f);
/**
 * manual control max pitch degree in rad during transintion
 *
 *
 *
 * @min 0
 * @max 1.57
 * @increment 0.01
 * @decimal 2
 * @group VTOL Attitude Control
 */
PARAM_DEFINE_FLOAT(ZXJ_MAN_PIT_MAX, 0.5f);
/**
 * manual control max roll degree in rad during transintion
 *
 *
 *
 * @min 0
 * @max 1.57
 * @increment 0.01
 * @decimal 2
 * @group VTOL Attitude Control
 */
PARAM_DEFINE_FLOAT(ZXJ_MAN_ROL_MAX, 0.4f);
