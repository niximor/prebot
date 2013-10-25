#
# PreBot IRC bot software
# (c) Michal Kuchta 2013
#

from src.event import handler
from datetime import datetime, timedelta
from time import mktime
import re
import logging

log = logging.getLogger(__name__)

def str_to_timedelta(text):
	duration = {
		"hours": 0,
		"minutes": 0,
		"seconds": 0,
		"days": 0,
	}

	# Parse duration
	hours_re = ["h","hodina","hodinu","hodin","hod","hour","hours","hr","hrs"]
	mins_re = ["m", "minuta","minutu","minut","min","minute","minutes","mins"]
	secs_re = ["s", "sekunda","sekundu","sekund","sec","second","seconds","secs","vterina","vterinu","vterin"]
	days_re = ["d", "den", "dny", "dnu", "dni", "day", "days"]
	weeks_re = ["w", "t", "tyden", "tydny", "tydnu", "week", "weeks"]
	years_re = ["r", "l", "y", "rok", "roky" "roku", "let", "year", "years"]

	matches = re.findall(r"(([0-9]+(\.[0-9]+)?)\s*(%s)\b)" % "|".join(hours_re + mins_re + secs_re + days_re + weeks_re +years_re), text)

	for match in matches:
		time = float(match[1])
		unit = match[3]

		if unit in hours_re:
			duration["hours"] += time
		elif unit in mins_re:
			duration["minutes"] += time
		elif unit in secs_re:
			duration["seconds"] += time
		elif unit in days_re:
			duration["days"] += time
		elif unit in weeks_re:
			duration["days"] += 7*time
		elif unit in years_re:
			duration["days"] += 365*time

	return timedelta(**duration)


def str_to_time(text, past=False):
	date = {}

	# Try to find date
	dates = re.search(r"(\d{1,2})\.\s*(\d{1,2})\.\s*(\d{4})?", text)
	if dates:
		date["day"] = int(dates.group(1))
		date["month"] = int(dates.group(2))
		date["year"] = int(dates.group(3))
	else:
		dates = re.search(r"(\d{4})-(\d{1,2})-(\d{1,2})", text)
		if dates:
			date["day"] = int(dates.group(3))
			date["month"] = int(dates.group(2))
			date["year"] = int(dates.group(1))
		else:
			dates = re.search(r"(\d{1,2})-(\d{1,2})-(\d{4})", text)
			if dates:
				date["day"] = int(dates.group(2))
				date["month"] = int(dates.group(1))
				date["year"] = int(dates.group(3))

	# Try to find time
	times = re.search(r"(\d{1,2}):(\d{1,2})(:(\d{1,2}))?", text)
	if times:
		date["hour"] = int(times.group(1))
		date["minute"] = int(times.group(2))

		if times.group(3) is not None:
			date["second"] = int(times.group(4))

	target = datetime.now().replace(**date)

	# If day has not been specified, can go to past.
	if "day" not in date and past:
		target -= timedelta(days=1)

	return target
	

def format_plural(num, choices):
	if num == 1:
		return choices[0]
	elif num >= 2 and num <= 4:
		return choices[1]
	else:
		return choices[2]


def format_delta(delta):
	secs = abs(round(delta.total_seconds()))
	to_format = []
	if secs >= 86400*365:
		# years
		years = int(secs / (86400 * 365))
		secs -= years * 86400 * 365
		to_format.append("%d %s" % (years, format_plural(years, ["rok", "roky", "let"])))

	if secs >= 86400:
		days = int(secs / 86400)
		secs -= days * 86400
		to_format.append("%d %s" % (days, format_plural(days, ["den", "dny", "dnu"])))

	if secs >= 3600:
		hours = int(secs / 3600)
		secs -= hours * 3600
		to_format.append("%d %s" % (hours, format_plural(hours, ["hodina", "hodiny", "hodin"])))

	if secs >= 60:
		mins = int(secs / 60)
		secs -= mins * 60
		to_format.append("%d %s" % (mins, format_plural(mins, ["minuta", "minuty", "minut"])))

	secs = int(secs)

	if secs > 0 or not len(to_format):
		log.debug("secs=%s, to_format=%s" % (secs, to_format))
		to_format.append("%d %s" % (secs, format_plural(secs, ["sekunda", "sekundy", "sekund"])))

	if len(to_format) > 1:
		return "%s a %s" % (", ".join(to_format[:-1]), to_format[-1])
	else:
		return to_format[0]


def format_time(time, useDate=True, useSeconds=True):
	out = []

	if useDate:
		out.append("%d.%d.%d " % (time.day, time.month, time.year))

	out.append("%d:%02d" % (time.hour, time.minute))

	if useSeconds:
		out.append(":%02d" % (time.second))

	return "".join(out)


@handler("command.cas-za")
def cas_za(eventData, handlerData):
	delta = str_to_timedelta(eventData.text)
	now = datetime.now()
	next = now + delta

	useDate = next.year != now.year or next.day != now.day or next.month != now.month
	useSeconds = (delta.total_seconds() % 60) > 0

	if delta.total_seconds() >= 0:
		eventData.parent.reply("Za %s bude %s." % (format_delta(delta),
								format_time(next, useDate=useDate, useSeconds=useSeconds)))
	else:
		eventData.parent.reply("Pred %s bylo %s." % (format_delta(delta),
								format_time(next, useDate=useDate, useSeconds=useSeconds)))


@handler("command.cas-pred")
def cas_pred(eventData, handlerData):
	delta = str_to_timedelta(eventData.text)
	delta = timedelta(seconds = -delta.total_seconds())

	now = datetime.now()
	next = now + delta

	useDate = next.year != now.year or next.day != now.day or next.month != now.month
	useSeconds = (delta.total_seconds() % 60) > 0

	if delta.total_seconds() >= 0:
		eventData.parent.reply("Za %s bude %s." % (format_delta(delta),
								format_time(next, useDate=useDate, useSeconds=useSeconds)))
	else:
		eventData.parent.reply("Pred %s bylo %s." % (format_delta(delta),
								format_time(next, useDate=useDate, useSeconds=useSeconds)))


@handler("command.cas-do")
def cas_do(eventData, handlerData):
	time = str_to_time(eventData.text)
	now = datetime.now()

	delta = now - time

	if now > time:
		eventData.parent.reply("%s uz bylo." % (format_time(time)))
	else:
		eventData.parent.reply("Do %s zbyva: %s" % (format_time(time), format_delta(delta)))


@handler("command.cas-od")
def cas_od(eventData, handlerData):
	time = str_to_time(eventData.text, past=True)
	now = datetime.now()

	delta = now - time

	if now > time:
		eventData.parent.reply("Od %s uplynulo: %s" % (format_time(time), format_delta(delta)))
	else:
		eventData.parent.reply("%s jeste nebylo." % (format_time(time)))


@handler("command.cas-od-do")
def cas_od_do(eventData, handlerData):
	text = eventData.text
	cnt = text.count("-")
	if cnt == 1:
		od, do = text.split("-")
	elif cnt == 0:
		return
	else:
		od, do = text.split(" - ")

	od = str_to_time(od)
	do = str_to_time(do)

	if od > do:
		temp = do
		do = od
		od = temp

	delta = do - od
	eventData.parent.reply("Mezi %s a %s je %s" % (format_time(od), format_time(do), format_delta(delta)))


@handler("command.cas-od-za")
def cas_od_za(eventData, handlerData):
	text = eventData.text
	cnt = text.count("-")
	if cnt == 1:
		od, za = text.split("-")
	elif cnt == 0:
		return
	else:
		od, za = text.split(" - ")

	od = str_to_time(od)
	za = str_to_timedelta(za)

	eventData.parent.reply("Za %s od %s bude %s" % (format_delta(za), format_time(od), format_time(od + za)))


@handler("command.time")
def time(eventData, handlerData):
	now = datetime.now()
	ts = int(mktime(now.timetuple()))
	eventData.parent.reply("Prave je %s :: Unix TS: %s" % (format_time(now), ts))

