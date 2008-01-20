#!/usr/bin/python
import cwiid
import sys

menu = '''1: toggle LED 1
2: toggle LED 2
3: toggle LED 3
4: toggle LED 4
5: toggle rumble
a: toggle accelerometer reporting
b: toggle button reporting
e: toggle extension reporting
i: toggle ir reporting
m: toggle messages
p: print this menu
r: request status message ((t) enables callback output)
s: print current state
t: toggle status reporting
x: exit'''

def main():
	led = 0
	rpt_mode = 0
	rumble = 0
	mesg = False

	#Connect to address given on command-line, if present
	print 'Put Wiimote in discoverable mode now (press 1+2)...'
	if len(sys.argv) > 1:
		wiimote = cwiid.Wiimote(sys.argv[1])
	else:
		wiimote = cwiid.Wiimote()

	wiimote.mesg_callback = callback

	print menu

	exit = 0
	while not exit:
		c = sys.stdin.read(1)
		if c == '1':
			led ^= cwiid.LED1_ON
			wiimote.led = led
		elif c == '2':
			led ^= cwiid.LED2_ON
			wiimote.led = led
		elif c == '3':
			led ^= cwiid.LED3_ON
			wiimote.led = led
		elif c == '4':
			led ^= cwiid.LED4_ON
			wiimote.led = led
		elif c == '5':
			rumble ^= 1
			wiimote.rumble = rumble
		elif c == 'a':
			rpt_mode ^= cwiid.RPT_ACC
			wiimote.rpt_mode = rpt_mode
		elif c == 'b':
			rpt_mode ^= cwiid.RPT_BTN
			wiimote.rpt_mode = rpt_mode
		elif c == 'e':
			rpt_mode ^= cwiid.RPT_EXT
			wiimote.rpt_mode = rpt_mode
		elif c == 'i':
			rpt_mode ^= cwiid.RPT_IR
			wiimote.rpt_mode = rpt_mode
		elif c == 'm':
			mesg = not mesg
			if mesg:
				wiimote.enable(cwiid.FLAG_MESG_IFC);
			else:
				wiimote.disable(cwiid.FLAG_MESG_IFC);
		elif c == 'p':
			print menu
		elif c == 'r':
			wiimote.request_status()
		elif c == 's':
			print_state(wiimote.state)
		elif c == 't':
			rpt_mode ^= cwiid.RPT_STATUS
			wiimote.rpt_mode = rpt_mode
		elif c == 'x':
			exit = -1;
		elif c == '\n':
			pass
		else:
			print 'invalid option'

	wiimote.close()

def print_state(state):
	print 'Report Mode:',
	for r in ['STATUS', 'BTN', 'ACC', 'IR', 'NUNCHUK', 'CLASSIC']:
		if state['rpt_mode'] & eval('cwiid.RPT_' + r):
			print r,
	print

	print 'Active LEDs:',
	for led in ['1','2','3','4']:
		if state['led'] & eval('cwiid.LED' + led + '_ON'):
			print led,
	print

	print 'Rumble:', state['rumble'] and 'On' or 'Off'

	print 'Battery:', int(100.0 * state['battery'] / cwiid.BATTERY_MAX)

	if 'buttons' in state:
		print 'Buttons:', state['buttons']

	if 'acc' in state:
		print 'Acc: x=%d y=%d z=%d' % (state['acc'][cwiid.X],
		                               state['acc'][cwiid.Y],
		                               state['acc'][cwiid.Z])

	if 'ir_src' in state:
		valid_src = False
		print 'IR:',
		for src in state['ir_src']:
			if src:
				valid_src = True
				print src['pos'],

		if not valid_src:
			print 'no sources detected'
		else:
			print

	if state['ext_type'] == cwiid.EXT_NONE:
		print 'No extension'
	elif state['ext_type'] == cwiid.EXT_UNKNOWN:
		print 'Unknown extension attached'
	elif state['ext_type'] == cwiid.EXT_NUNCHUK:
		print 'Nunchuk: btns=%.2X stick=%r acc.x=%d acc.y=%d acc.z=%d' % \
		  (state['nunchuk']['buttons'], state['nunchuk']['stick'],
		   state['nunchuk']['acc'][cwiid.X],
		   state['nunchuk']['acc'][cwiid.Y],
		   state['nunchuk']['acc'][cwiid.Z])
	elif state['ext_type'] == cwiid.EXT_CLASSIC:
		print 'Classic: btns=%.4X l_stick=%r r_stick=%r l=%d r=%d' % \
		  (state['classic']['buttons'],
		   state['classic']['l_stick'], state['classic']['r_stick'],
		   state['classic']['l'], state['classic']['r'])

def callback(mesg_list):
	for mesg in mesg_list:
		if mesg[0] == cwiid.MESG_STATUS:
			print 'Status Report: battery=%d extension=' % \
			       mesg[1]['battery'],
			if mesg[1]['ext_type'] == cwiid.EXT_NONE:
				print 'none'
			elif mesg[1]['ext_type'] == cwiid.EXT_NUNCHUK:
				print 'Nunchuk'
			elif mesg[1]['ext_type'] == cwiid.EXT_CLASSIC:
				print 'Classic Controller'
			else:
				print 'Unknown Extension'

		elif mesg[0] == cwiid.MESG_BTN:
			print 'Button Report: %.4X' % mesg[1]

		elif mesg[0] == cwiid.MESG_ACC:
			print 'Acc Report: x=%d, y=%d, z=%d' % \
			      (mesg[1][cwiid.X], mesg[1][cwiid.Y], mesg[1][cwiid.Z])

		elif mesg[0] == cwiid.MESG_IR:
			valid_src = False
			print 'IR Report: ',
			for src in state['ir_src']:
				if src:
					valid_src = True
					print src['pos'],

			if not valid_src:
				print 'no sources detected'
			else:
				print

		elif mesg[0] == cwiid.MESG_NUNCHUK:
			print 'Nunchuk Report: btns=%.2X stick=%r ' + \
			      'acc.x=%d acc.y=%d acc.z=%d' % (state['nunchuk']['buttons'],
			   state['nunchuk']['stick'],
			   state['nunchuk']['acc'][cwiid.X],
			   state['nunchuk']['acc'][cwiid.Y],
			   state['nunchuk']['acc'][cwiid.Z])
		elif mesg[0] == cwiid.MESG_CLASSIC:
			print 'Classic Report: btns=%.4X l_stick=%r r_stick=%r' + \
			      'l=%d r=%d' % (state['classic']['buttons'],
			   state['classic']['l_stick'], state['classic']['r_stick'],
			   state['classic']['l'], state['classic']['r'])
		elif mesg[0] ==  cwiid.MESG_ERROR:
			wiimote.close()
			exit(-1)
		else:
			print 'Unknown Report'

main()
