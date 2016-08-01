
-- File creates tables and trigger functions for testlite database

-- Drop existing tables
DROP TABLE hk1;
DROP TABLE hk2;
DROP TABLE hk4;
DROP TABLE cmd;
DROP TABLE hd;
DROP TABLE wv;

-- Housekeeping1 DB table
CREATE TABLE hk1 (
	crc int4,      
	pcktype int4, 
	pcknum int8 primary key, 
	pctick int8,
	time int8, 
	ms int4, 
	dac0 int4, 
	dac1 int4, 
	dac2 int4, 
	dac3 int4, 
	dac4 int4,	
	adc0 int4, 
	adc1 int4, 
	adc2 int4,
	adc3 int4,
	adc4 int4,
	lint int2
);
CREATE INDEX hk1_time_index ON hk1 (time);

-- Housekeeping2 DB table
CREATE TABLE hk2 (
	crc int4,      
	pcktype int4, 
	pcknum int8 primary key, 
	pctick int8,
	time int8, 
	ms int4,
	ssx0 int4,
	ssy0 int4,
	ssi0 int4,
	sst0 int4, 
	ssx1 int4,
	ssy1 int4,
	ssi1 int4,
	sst1 int4, 
	ssx2 int4,
	ssy2 int4,
	ssi2 int4,
	sst2 int4, 
	ssx3 int4,
	ssy3 int4,
	ssi3 int4,
	sst3 int4, 
	temp0 int4,
	temp1 int4,
	pres0 int4,
	pres1  int4
);
CREATE INDEX hk2_time_index ON hk2 (time);

-- Housekeeping4 DB table
CREATE TABLE hk4 (
	crc int4,      
	pcktype int4, 
	pcknum int8 primary key, 
	pctick int8,
	time int8, 
	pckw0 int4,
	pckw1 int4,
	pckw2 int4,
	pckw3 int4,
	pckw4 int4,
	pckw5 int4,
	pckw6 int4,
	pckw7 int4,
	pckw8 int4,
	pckw9 int4,
	pckwa int4,
	pckwb int4,
	pckwc int4,
	pckwd int4,
	pckwe int4,
	pckwf int4,
	pcka0 int4,
	pcka1 int4,
	pcka2 int4,
	pcka3 int4,
	pcka4 int4,
	pcka5 int4,
	pcka6 int4,
	pcka7 int4,
	pcka8 int4,
	pcka9 int4,
	pckaa int4,
	pckab int4,
	pckac int4,
	pckad int4,
	pckae int4,
	pckaf int4,
	evw0 int4,
	evw1 int4,
	evw2 int4,
	evw3 int4,
	evw4 int4,
	evw5 int4,
	evw6 int4,
	evw7 int4,
	evw8 int4,
	evw9 int4,
	eva0 int4,
	eva1 int4,
	eva2 int4,
	eva3 int4,
	eva4 int4,
	eva5 int4,
	eva6 int4,
	eva7 int4,
	eva8 int4,
	eva9 int4
);
CREATE INDEX hk4_time_index ON hk4 (time);

-- Command DB table
CREATE TABLE cmd (
	crc int4,      
	pcktype int4, 
	pcknum int8 primary key, 
	pctick int8,
	time int8,
	cs   int4,
	cmd  char(8),
	ret  int8
);	
CREATE INDEX cmd_time_index ON cmd (time);

-- Header DB table
CREATE TABLE hd (
	crc int4,      
	pcktype int4, 
	pcknum int8 primary key, 
	pctick int8,
	evnum int8,
	trig int2,
	time int4,
	ms int2,
	sampint int4,
	trgdel int4,
	nsamp int4,
	nseg int2,
	holdoff int2,
	dtype int2,
	trgcouple int2,
	trgslope int2 CHECK (trgslope=0 OR trgslope=1),
	trgsource int2 CHECK (trgsource=-1 OR trgsource=-2 OR trgsource=-3 OR trgsource>0),
	trglevel int2,
	clocktype int2 CHECK (clocktype=4 OR clocktype=2 OR clocktype=1 OR clocktype=0),
	chfs0 int2,
	chfs1 int2,
	chfs2 int2,
	chfs3 int2,
	choffs0 int2,
	choffs1 int2,
	choffs2 int2,
	choffs3 int2,
	chcouple0 int2,
	chcouple1 int2,
	chcouple2 int2,
	chcouple3 int2,
	chbw0 int2,
	chbw1 int2,
	chbw2 int2,
	chbw3 int2,
	temp0 int2,
	temp1 int2,
	chmean0 int2,
	chmean1 int2,
	chmean2 int2,
	chmean3 int2,
	chsdev0 int2,
	chsdev1 int2,
	chsdev2 int2,
	chsdev3 int2,
	gpsday int2,
	gpshour int2,
	gpsmin int2,
	gpssec int2,
	gpsfsec int4,
	gpslat real,
	gpslong real,
	gpsalt real,
	meanpwr0 int2,
	meanpwr1 int2,
	meanpwr2 int2,
	meanpwr3 int2,
	peakpwr0 int2,
	peakpwr1 int2,
	peakpwr2 int2,
	peakpwr3 int2,
	peakf0 int2,
	peakf1 int2,
	peakf2 int2,
	peakf3 int2,
	priority int2
);
CREATE INDEX hd_time_index ON hd (time);


-- Event DB table
CREATE TABLE wv (
	crc int4,      
	pcktype int4, 
	pcknum int8 primary key, 
	pctick int8,
	evnum int8,
	trig int2,
	time int4,
	ms int2,
	hk1 int4,
	hk2 int4,
	hk4 int4,
	cmd int4,
	ch1 int2[],
	ch2 int2[],
	ch3 int2[],
	ch4 int2[]
);
CREATE INDEX wv_time_index ON wv (time);


CREATE OR REPLACE FUNCTION newwv() RETURNS trigger AS '
  DECLARE 
     wvrec RECORD;
     dt integer := 60;
     refpt integer;
     refnt integer;
  BEGIN
-- Must have time reference 
     IF NEW.time IS NULL THEN
         RAISE EXCEPTION ''%: time cannot be null'',NEW.evnum;
     END IF;

-- If update w/o time change, no action
     IF TG_OP = ''UPDATE'' THEN
       IF NEW.time = OLD.time THEN
         RETURN NEW;
       END IF;
     END IF;

-- If insertion, then check if entry already exists
    IF TG_OP = ''INSERT'' THEN
       SELECT INTO wvrec * FROM wv WHERE (time=NEW.time AND ms=NEW.ms);
       IF FOUND THEN -- Event already exists, so ignore this entry
	  RETURN NULL;
       END IF;
     END IF;

-- Find nearby hk1 event
     SELECT INTO refnt MIN(NEW.time-time) FROM hk1 WHERE (time>(NEW.time-dt) AND time<=NEW.time);
     IF NOT FOUND THEN
	 refnt:=-1;
     END IF;
     SELECT INTO refpt MIN(time-NEW.time) FROM hk1 WHERE (time>=NEW.time AND time<(NEW.time+dt));
     IF NOT FOUND THEN
	 refpt:=-1;
     END IF;
     IF refnt>=0 AND refpt>=0 THEN -- Events found on both sides
	IF refnt<refpt THEN
	   NEW.hk1 = NEW.time-refnt;
        ELSE
           NEW.hk1 = NEW.time+refpt;
        END IF;
     ELSIF refnt>=0 THEN  -- Only preceeding events found
        NEW.hk1 = NEW.time-refnt;
     ELSIF refpt>=0 THEN  -- Only trailing events found
        NEW.hk1 = NEW.time+refpt;
     END IF;

-- Find nearby hk2 event
     SELECT INTO refnt MIN(NEW.time-time) FROM hk2 WHERE (time>(NEW.time-dt) AND time<=NEW.time);
     IF NOT FOUND THEN
	 refnt:=-1;
     END IF;
     SELECT INTO refpt MIN(time-NEW.time) FROM hk2 WHERE (time>=NEW.time AND time<(NEW.time+dt));
     IF NOT FOUND THEN
	 refpt:=-1;
     END IF;
     IF refnt>=0 AND refpt>=0 THEN -- Events found on both sides
	IF refnt<refpt THEN
	   NEW.hk2 = NEW.time-refnt;
        ELSE
           NEW.hk2 = NEW.time+refpt;
        END IF;
     ELSIF refnt>=0 THEN  -- Only preceeding events found
        NEW.hk2 = NEW.time-refnt;
     ELSIF refpt>=0 THEN  -- Only trailing events found
        NEW.hk2 = NEW.time+refpt;
     END IF;

-- Find nearby hk4 event
     SELECT INTO refnt MIN(NEW.time-time) FROM hk4 WHERE (time>(NEW.time-dt) AND time<=NEW.time);
     IF NOT FOUND THEN
	 refnt:=-1;
     END IF;
     SELECT INTO refpt MIN(time-NEW.time) FROM hk4 WHERE (time>=NEW.time AND time<(NEW.time+dt));
     IF NOT FOUND THEN
	 refpt:=-1;
     END IF;
     IF refnt>=0 AND refpt>=0 THEN -- Events found on both sides
	IF refnt<refpt THEN
	   NEW.hk4 = NEW.time-refnt;
        ELSE
           NEW.hk4 = NEW.time+refpt;
        END IF;
     ELSIF refnt>=0 THEN  -- Only preceeding events found
        NEW.hk4 = NEW.time-refnt;
     ELSIF refpt>=0 THEN  -- Only trailing events found
        NEW.hk4 = NEW.time+refpt;
     END IF; 

-- Find nearby cmd event
     SELECT INTO refpt MAX(time) FROM cmd WHERE (time<NEW.time);
     IF FOUND THEN
         NEW.cmd:=refpt;
     END IF;
     RETURN NEW;
  END;
' LANGUAGE plpgsql;

-- Checks if waveform already present and splices with new information if needed
CREATE OR REPLACE FUNCTION chsplice() RETURNS trigger AS '
  DECLARE
    wvrec RECORD; 
    n integer;
  BEGIN
    SELECT INTO wvrec * FROM wv WHERE (time=NEW.time AND ms=NEW.ms);
    IF NOT FOUND THEN -- This is first entry
       RETURN NEW;
    END IF;

    -- This is not first entry for the same event, thus splice existing waveforms
    SELECT INTO n nsamp FROM hd WHERE (time=NEW.time AND ms=NEW.ms);
    IF NOT FOUND OR n is NULL THEN 
       n := 1024;
    END IF;
    FOR j in 1..n LOOP
        IF wvrec.ch1[j] <> 0 AND NEW.ch1[j] = 0 THEN  
	  NEW.ch1[j]=wvrec.ch1[j];
        END IF;
        IF wvrec.ch2[j] <> 0 AND NEW.ch2[j] = 0 THEN  
   	  NEW.ch2[j]=wvrec.ch2[j];
        END IF;
        IF wvrec.ch3[j] <> 0 AND NEW.ch3[j] = 0 THEN  
	  NEW.ch3[j]=wvrec.ch3[j];
        END IF;
        IF wvrec.ch4[j] <> 0 AND NEW.ch4[j] = 0 THEN  
 	  NEW.ch4[j]=wvrec.ch4[j];
        END IF;
    END LOOP;

    RETURN NEW;
  END;
' LANGUAGE plpgsql;

CREATE TRIGGER newwv BEFORE INSERT OR UPDATE ON wv
    FOR EACH ROW EXECUTE PROCEDURE newwv();
CREATE TRIGGER chsplice BEFORE UPDATE ON wv
    FOR EACH ROW EXECUTE PROCEDURE chsplice();

CREATE OR REPLACE FUNCTION newhk1() RETURNS trigger AS '
  DECLARE 
     dt integer := 60;
     wvrec RECORD;
  BEGIN
     FOR wvrec IN SELECT time,hk1 FROM wv WHERE (time>(NEW.time-dt) AND time<(NEW.time+dt)) LOOP
         IF ABS(wvrec.time-NEW.time)<ABS(wvrec.time-wvrec.hk1) OR wvrec.hk1 IS NULL THEN
           UPDATE wv SET hk1=NEW.time WHERE time=wvrec.time;
         END IF;
     END LOOP;
     RETURN NEW;
  END;
' LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION newhk2() RETURNS trigger AS '
  DECLARE 
     dt integer := 60;
     wvrec RECORD;
  BEGIN
     FOR wvrec IN SELECT time,hk2 FROM wv WHERE (time>(NEW.time-dt) AND time<(NEW.time+dt)) LOOP
         IF ABS(wvrec.time-NEW.time)<ABS(wvrec.time-wvrec.hk2) OR wvrec.hk2 IS NULL THEN
           UPDATE wv SET hk2=NEW.time WHERE time=wvrec.time;
         END IF;
     END LOOP;
     RETURN NEW;
  END;
' LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION newhk4() RETURNS trigger AS '
  DECLARE 
     dt integer := 60;
     wvrec RECORD;
  BEGIN
     FOR wvrec IN SELECT time,hk4 FROM wv WHERE (time>(NEW.time-dt) AND time<(NEW.time+dt)) LOOP
         IF ABS(wvrec.time-NEW.time)<ABS(wvrec.time-wvrec.hk4) OR wvrec.hk4 IS NULL THEN
           UPDATE wv SET hk4=NEW.time WHERE time=wvrec.time;
         END IF;
     END LOOP;
     RETURN NEW;
  END;
' LANGUAGE plpgsql;

CREATE TRIGGER newhk1 AFTER INSERT OR UPDATE ON hk1
    FOR EACH ROW EXECUTE PROCEDURE newhk1();
CREATE TRIGGER newhk2 AFTER INSERT OR UPDATE ON hk2
    FOR EACH ROW EXECUTE PROCEDURE newhk2();
CREATE TRIGGER newhk4 AFTER INSERT OR UPDATE ON hk4
    FOR EACH ROW EXECUTE PROCEDURE newhk4();

CREATE OR REPLACE FUNCTION newcmd() RETURNS trigger AS '
  DECLARE 
     wvrec RECORD;
  BEGIN
     FOR wvrec IN SELECT time,cmd FROM wv WHERE (time>=NEW.time) LOOP
         IF wvrec.time-NEW.time<wvrec.time-wvrec.cmd OR wvrec.cmd IS NULL THEN
           UPDATE wv SET cmd=NEW.time WHERE time=wvrec.time;
         END IF;
     END LOOP;
     RETURN NEW;
  END;
' LANGUAGE plpgsql;

CREATE TRIGGER newcmd AFTER INSERT OR UPDATE ON cmd
    FOR EACH ROW EXECUTE PROCEDURE newcmd();

-- Create user apache and allow read access to all tables for web access
CREATE USER apache;
GRANT SELECT ON hk1 TO apache;
GRANT SELECT ON hk2 TO apache;
GRANT SELECT ON hk4 TO apache;
GRANT SELECT ON cmd TO apache;
GRANT SELECT ON hd TO apache;
GRANT SELECT ON wv TO apache;

-- Create user anita and allow read access to all tables for viewer access
CREATE USER anita;
GRANT SELECT ON hk1 TO anita;
GRANT SELECT ON hk2 TO anita;
GRANT SELECT ON hk4 TO anita;
GRANT SELECT ON cmd TO anita;
GRANT SELECT ON hd TO anita;
GRANT SELECT ON wv TO anita;
