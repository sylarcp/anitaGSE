
-- File creates tables and trigger functions for ANITA flight database
-- TODO insert NOTIFY for user end table entries...

-- User should use 'hk' view to access housekeeping data

-- CRC entries are two byte; top byte gives wrapper CRC quality, bottom byte internal CRC quality
-- In both cases 0=failed CRC check, 1=passed CRC check

-- Drop existing tables
DROP TABLE hd CASCADE;   -- Event header table 
DROP TABLE wv CASCADE;     -- Waveform table, one channel per entry, raw and calibrated (this is a big table!!!!)
DROP TABLE hk CASCADE;      -- Calibrated housekeeping data table
DROP TABLE hk_raw CASCADE;  -- Raw housekeeping data table, with calib constants
DROP TABLE hk_surf CASCADE; -- Housekeeping data reported by SURFs
DROP TABLE turf CASCADE;    -- Trigger rates reported by TURF
DROP TABLE mon;     -- Disk usage and queue status information
DROP TABLE adu5_pat CASCADE;  -- Instrument position table as reported by ADU5 gps
DROP TABLE adu5_sat; -- GPS satellite information reported by ADU5 gps
DROP TABLE adu5_vtg; -- Velocity information from ADU5 gps
DROP TABLE g12_pos; -- Instrument position and velocity from G12 gps
DROP TABLE g12_sat; -- GPS satellite infromation from G12 gps
DROP TABLE wv_ped;   -- Table with channel pedestals
DROP TABLE cmd;      -- Command echo table
DROP TABLE wakeup;   -- Wake up packets, just log appearances

-- Raw housekeeping table
CREATE TABLE hk_raw (
	nbuf int8 PRIMARY KEY,
	crc int2,   -- see CRC comment at the top
	now int8,   -- time of processing
	time int8, 
	us int8, 
	code int2 CHECK (code=256 OR code=512 OR code=768),
	bd1  int2[],    -- Board 1 data 
	bd2  int2[],    -- Board 2 data
	bd3  int2[],    -- Board 3 data
	sbst1 int4,     -- SBS CPU temperatures
        sbst2 int4,  
	magx  real,   -- Magnetometer info 
	magy  real, 
	magz  real,
	UNIQUE (time,us,code)
);
-- These are default AVZ and CAL calibrations from EM flight.... 
COPY hk_raw (nbuf,time,us,code,bd1,bd2,bd3) FROM '/tmp/hkdValues.txt' WITH DELIMITER AS ' ';
--\copy hk_raw (time,us,code,bd1,bd2,bd3) from '/tmp/hkdValues.txt' with delimiter ' '
CREATE INDEX hk_raw_time_index ON hk_raw (time);

-- Calibrated housekeeping table
CREATE TABLE hk (
	nbuf int8 REFERENCES hk_raw PRIMARY KEY,
	time int8, 
	us int8, 
	now int8, 
	cal int8 REFERENCES hk_raw,   -- References nbuf of CAL code hk packet  
	avz int8 REFERENCES hk_raw,   -- References nbuf of AVZ code hk packet  
	bd1  real[],    -- Board 1 data 
	bd2  real[],    -- Board 2 data
	bd3  real[],    -- Board 3 data
	accx real[],  -- Accelerometer, x-axes
	accy real[],  -- Accelerometer, y-axes
	accz real[],  -- Accelerometer, z-axes
	acct real[],  -- Accelerometer, temperatures
	ssx real[],    -- Sunsensor X-coordinates
	ssy real[],    -- Sunsensor Y-coordinates
	sst real[],    -- Sunsensor temperatures
	pressh real, -- Pressure sensor high
        pressl real, -- Pressure sensor low
	p1_5v real,  -- +1.5V true value
	p3_3v real,  -- +3.3V true value
	p5v real,    -- +5V true value
	p5sbv real,  -- +5V short board true value 
	p12v real,   -- +12V true value
	p24v real,   -- +24V voltage
	ppvv real,   -- +PV voltage
	n5v real,    -- -5V true value
	n12v real,   -- -12V true value
	iprf1v real,  -- RFCM1 voltage
	iprf2v real,  -- RFCM2 voltage
	p1_5i real,  -- +1.5V supply current
	p3_3i real,  -- +3.3V supply current
	p5i real,    -- +5V supply current
	p5sbi real,  -- +5V short board supply current
	p12i real,   -- +12V supply current
	p24i real,   -- +24V supply current
	ppvi real,   -- +PV supply current
	n5i real,    -- -5V supply current
	n12i real,   -- -12V supply current
	iprf1i real,  -- RFCM1 supply current 
	iprf2i real,  -- RFCM2 supply current 
	bati real,    -- battery current
	it real[], -- Internal temperatures in Celsius
	et real[],  -- External temperatures in Celsius
	sbst1 int4,     -- SBS CPU temperatures
        sbst2 int4,  
	magx  real,   -- Magnetometer info 
	magy  real, 
	magz  real,
	UNIQUE(time,us)
);
CREATE INDEX hk_time_index ON hk (time);

-- Create function which will calculate voltages for HK Acromag entries
-- and calibrate physical values
CREATE OR REPLACE FUNCTION calhk() RETURNS trigger AS '
  DECLARE 
    caltime int8;
    avztime int8;
    hkcal RECORD;
    hkavz RECORD;
    calb1 real[]:=''{}''; -- Acromag voltages per board	
    calb2 real[]:=''{}'';	
    calb3 real[]:=''{}'';	 
    slope real;    -- Temp variable
    accx real[]:=''{}'';  -- Accelerometers, x-axes in m/s
    accy real[]:=''{}'';  -- Accelerometers, y-axes in m/s
    accz real[]:=''{}'';  -- Accelerometers, z-axes in m/s
    acct real[]:=''{}'';  -- Accelerometers, temperatures in C
    ssx real[]:=''{}'';    -- Sunsensors X-coordinates
    ssy real[]:=''{}'';    -- Sunsensors Y-coordinates
    sst real[]:=''{}'';    -- Sunsensors temperatures in C
    pressh real; -- Pressure sensor high in PSI
    pressl real; -- Pressure sensor low	in Torr
    p1_5v real;  -- +1.5V true value
    p3_3v real;  -- +3.3V true value
    p5v real;    -- +5V true value
    p5sbv real;  -- +5V short boards true value
    p12v real;   -- +12V true value
    p24v real;   -- +24V voltage
    ppvv real;   -- +PV voltage
    n5v real;    -- -5V true value
    n12v real;   -- -12V true value
    iprf1v real;  -- RFCM1 voltage
    iprf2v real;  -- RFCM2 voltage
    p1_5i real;  -- +1.5V supply current in amps
    p3_3i real;  -- +3.3V supply current in amps
    p5i real;    -- +5V supply current in amps
    p5sbi real;  -- +5V short board supply current in amps
    p12i real;   -- +12V supply current in amps
    p24i real;   -- +24V supply current in amps
    ppvi real;   -- +PV supply current in amps
    n5i real;    -- -5V supply current in amps
    n12i real;   -- -12V supply current in amps
    iprf1i real;  -- RFCM1 supply current in amps
    iprf2i real;  -- RFCM2 supply current in amps
    bati real;   -- battery current in amps
    it real[]=''{}'';     -- Internal temps in Celsius
    et real[]=''{}'';     -- External temps in Celsius
  BEGIN
  -- No calibration, this is calibration data itself
     IF NEW.code=768 OR NEW.code=512 THEN 
       RETURN NEW;
     END IF;
  -- Find last CAL packet
     SELECT INTO caltime MAX(time) FROM hk_raw WHERE (code=768 AND (time<NEW.time OR (time=NEW.time AND us<NEW.us)));
     IF NOT FOUND THEN
    	RAISE EXCEPTION ''No CAL hk packet before time %d'', NEW.time;
     END IF;
     SELECT INTO hkcal * FROM hk_raw WHERE time=caltime AND code=768;

  -- Find last AVZ packet
     SELECT INTO avztime MAX(time) FROM hk_raw WHERE (code=512 AND (time<NEW.time OR (time=NEW.time AND us<NEW.us)));
     IF NOT FOUND THEN
    	RAISE EXCEPTION ''No AVZ hk packet before time %d'', NEW.time;
     END IF;
     SELECT INTO hkavz * FROM hk_raw WHERE time=avztime AND code=512;

     FOR i IN 1..40 LOOP
	  slope := 4.9 / (hkcal.bd1[i]-hkavz.bd1[i]);
	  calb1[i] := slope*(NEW.bd1[i] - hkavz.bd1[i]);

          slope := 4.9 / (hkcal.bd2[i]-hkavz.bd2[i]);
	  calb2[i] := slope*(NEW.bd2[i] - hkavz.bd2[i]);

          slope := 4.9 / (hkcal.bd3[i]-hkavz.bd3[i]);
	  calb3[i] := slope*(NEW.bd3[i] - hkavz.bd3[i]);

     END LOOP;

-- Create physical values based on documentation
     IF calb2[7]<>0 THEN
       accx[1]:=((2.5*calb2[31])/calb2[7]-2.493)/0.830;	
       accy[1]:=((2.5*calb2[11])/calb2[7]-2.512)/0.839;	
       accz[1]:=((2.5*calb2[30])/calb2[7]-2.506)/0.842;	
       accx[2]:=((2.5*calb2[29])/calb2[7]-2.523)/0.827;	
       accy[2]:=((2.5*calb2[9])/calb2[7]-2.523)/0.844;	
       accz[2]:=((2.5*calb2[28])/calb2[7]-2.523)/0.843;	
       acct[1]:=44.4*((2.5*calb2[10])/calb2[7]-1.375);	
       acct[2]:=44.4*((2.5*calb2[8])/calb2[7]-1.375);	
     END IF;

     IF (calb1[1]+calb1[21])<>0 THEN
       ssx[1]:=(calb1[1]-calb1[21])/(calb1[1]+calb1[21]);
     END IF;
     IF (calb1[2]+calb1[22])<>0 THEN
       ssy[1]:=(calb1[2]-calb1[22])/(calb1[2]+calb1[22]);	
     END IF;	
     sst[1]:=calb1[3]*100-273.15;

     IF (calb1[23]+calb1[4])<>0 THEN
       ssx[2]:=(calb1[23]-calb1[4])/(calb1[23]+calb1[4]);
     END IF;
     IF (calb1[24]+calb1[5])<>0 THEN
       ssy[2]:=(calb1[24]-calb1[5])/(calb1[24]+calb1[5]);	
     END IF;	
     sst[2]:=calb1[25]*100-273.15;

     IF (calb1[27]+calb1[8])<>0 THEN
       ssx[3]:=(calb1[27]-calb1[8])/(calb1[27]+calb1[8]);
     END IF;
     IF (calb1[28]+calb1[9])<>0 THEN
       ssy[3]:=(calb1[28]-calb1[9])/(calb1[28]+calb1[9]);	
     END IF;	
     sst[3]:=calb1[29]*100-273.15;

     IF (calb1[30]+calb1[10])<>0 THEN
       ssx[4]:=(calb1[30]-calb1[10])/(calb1[30]+calb1[10]);
     END IF;
     IF (calb1[11]+calb1[31])<>0 THEN
       ssy[4]:=(calb1[11]-calb1[31])/(calb1[11]+calb1[31]);	
     END IF;	
     sst[4]:=calb1[12]*100-273.15;

     pressh:=(calb2[32]-0.25)*3.75;  
     pressl:=0.00215181+4.0062*calb2[12]-0.0027642*calb2[12]*calb2[12];
     p1_5v:= 	    calb2[2];
     p3_3v:= 	    calb2[6];
     p5v:= 	    calb2[7]*2;
     p5sbv:= 	    calb2[3]*2;
     p12v:= 	    calb2[5]*4;
     p24v:= 	    calb2[37]*10.1377;
     ppvv:= 	    calb2[38]*18.252;
     n5v:= 	    calb2[1]*-2;
     n12v:= 	    calb2[4]*-4;
     iprf1v:= 	    calb2[34]*4;
     iprf2v:= 	    calb2[35]*4;
     p1_5i:= 	    calb2[21]*0.8;
     p3_3i:= 	    calb2[25]*8;
     p5i:= 	    calb2[26]*20;
     p5sbi:= 	    calb2[22]*12;
     p12i:= 	    calb2[24]*0.8;
     p24i:= 	    calb2[17]*20;
     ppvi:= 	    calb2[18]*20;
     n5i:= 	    calb2[27]*0.8;
     n12i:= 	    calb2[23]*0.8;
     iprf1i:=	    calb2[15]*8;	
     iprf2i:=	    calb2[14]*8;	
     bati:= 	    calb2[16]*20;

     it[1]:= 	    calb3[1]*100-273.15;
     it[2]:= 	    calb3[21]*100-273.15;
     it[3]:= 	    calb3[2]*100-273.15;
     it[4]:= 	    calb3[22]*100-273.15;
     it[5]:= 	    calb3[3]*100-273.15;
     it[6]:= 	    calb3[23]*100-273.15;
     it[7]:= 	    calb3[4]*100-273.15;
     it[8]:= 	    calb3[24]*100-273.15;
     it[9]:= 	    calb3[5]*100-273.15;
     it[10]:= 	    calb3[25]*100-273.15;
     it[11]:= 	    calb3[6]*100-273.15;
     it[12]:= 	    calb3[26]*100-273.15;
     it[13]:= 	    calb3[7]*100-273.15;
     it[14]:= 	    calb3[27]*100-273.15;
     it[15]:= 	    calb3[8]*100-273.15;
     it[16]:= 	    calb3[28]*100-273.15;
     it[17]:= 	    calb3[9]*100-273.15;
     it[18]:= 	    calb3[29]*100-273.15;   
     it[19]:= 	    calb3[10]*100-273.15;   
     it[20]:= 	    calb3[30]*100-273.15;
     it[21]:= 	    calb3[11]*100-273.15;
     it[22]:= 	    calb3[31]*100-273.15;
     it[23]:= 	    calb3[12]*100-273.15;
     it[24]:= 	    calb3[32]*100-273.15;
     it[25]:= 	    calb3[13]*100-273.15;

     et[1]:= 	    calb3[33]*100-273.15;
     et[2]:= 	    calb3[14]*100-273.15;
     et[3]:= 	    calb3[34]*100-273.15;
     et[4]:= 	    calb3[15]*100-273.15;
     et[5]:= 	    calb3[35]*100-273.15;
     et[6]:= 	    calb3[16]*100-273.15;
     et[7]:= 	    calb3[36]*100-273.15;
     et[8]:= 	    calb3[17]*100-273.15;
     et[9]:= 	    calb3[37]*100-273.15;
     et[10]:= 	    calb3[18]*100-273.15;
     et[11]:= 	    calb3[38]*100-273.15;
     et[12]:= 	    calb3[19]*100-273.15;
     et[13]:= 	    calb3[39]*100-273.15;
     et[14]:= 	    calb3[20]*100-273.15;   
     et[15]:= 	    calb3[40]*100-273.15;
     et[16]:= 	    calb1[16]*100-273.15;
     et[17]:= 	    calb1[36]*100-273.15;
     et[18]:= 	    calb1[17]*100-273.15;
     et[19]:= 	    calb1[37]*100-273.15;
     et[20]:= 	    calb1[18]*100-273.15;
     et[21]:= 	    calb1[38]*100-273.15;
     et[22]:= 	    calb1[19]*100-273.15;
     et[23]:= 	    calb1[39]*100-273.15;
     et[24]:= 	    calb1[20]*100-273.15;   
     et[25]:= 	    calb1[40]*100-273.15;	

-- Store calibrated values
     INSERT INTO hk VALUES (NEW.nbuf,NEW.time,NEW.us,NEW.now,
	    hkcal.nbuf,hkavz.nbuf,
	    calb1,calb2,calb3,
	    accx,accy,accz,acct,
	    ssx,ssy,sst,
            pressh,pressl,
	    p1_5v,p3_3v,p5v,p5sbv,p12v,p24v,ppvv,n5v,n12v,iprf1v,iprf2v,
	    p1_5i,p3_3i,p5i,p5sbi,p12i,p24i,ppvi,n5i,n12i,iprf1i,iprf2i,bati,
	    it,et,
	    NEW.sbst1,NEW.sbst2,NEW.magx,NEW.magy,NEW.magz);
     RETURN NEW;
  END;
' LANGUAGE plpgsql;
CREATE TRIGGER calhk AFTER INSERT OR UPDATE ON hk_raw
    FOR EACH ROW EXECUTE PROCEDURE calhk();

-- SURF housekeeping table
CREATE TABLE hk_surf (
	nbuf int8 PRIMARY KEY,
	crc int2,   -- See comment on top
	now int8,   -- time of processing
	time int8, 
	us int8, 
	global int4, -- Global threshold
	error  int4, -- Error flag
	upper int4[], -- Upper words, one per SURF
	scaler int4[][], -- Scaler values, 32 per SURF
	thresh int4[][], -- Threshold values, 32 per SURF
	rfpow  int4[][], -- RF power values, 8 per SURF	
	surfmask int4[], -- Surf RF trigger band mask (32-bit fields)
	UNIQUE (time,us)
);
CREATE INDEX hk_surf_time_index ON hk_surf (time);

-- TURF rates table
CREATE TABLE turf (
	nbuf int8 PRIMARY KEY,
	crc int2,   -- See comment on top
	now int8,   -- time of processing
	time int8, 
	us int8, 
	l1 int4[][], -- Level 1, antenna trigger rates (8 surfs by 4 antennas)
	l2 int2[][], -- Level 2 rate (2 layers, 16 sectors)
	l3 int2[],   -- Level 3 rate (16 sectors)
	UNIQUE (time,us)
);
CREATE INDEX turf_time_index ON turf (time);

-- Disk space and queue monitor table
CREATE TABLE mon (
	nbuf int8 PRIMARY KEY,
	crc int2,   -- See comment on top
	now int8,   -- time of processing
	time int8 UNIQUE, 
	main int4,  -- Main disk space
	other int4[], -- Secondary disks space
	usb  int4[], -- USB disk space (up to 64 disks)
	linkev int4[], -- Events in queue (10 priority queues)
	linkcmd int4, -- Command echos in queue
	linkgps int4, -- GPS packets in queue
	linkhk  int4, -- Housekeeping links in queue
	linkmon int4, -- Monitor links in queue
	linkhd  int4, -- Header links in queue
	linksurf int4, -- SURF hk links in queue
	linkturf int4, -- TURF rate links in queue
	linkforced int4 -- Forced event (?) links in queue 
);	
CREATE INDEX mon_time_index ON mon (time);


-- ADU5 GPS position table
CREATE TABLE adu5_pat (
	nbuf int8 PRIMARY KEY,
	crc int2,   -- See comment on top   
	now int8,   -- time of processing
	time int8, 
	us int8,
	tod int8,        -- Time of Day in ms
	heading real,  -- Payload heading in degrees
        pitch real,    -- Payload pitch in degrees
        roll real,     -- Payload roll in degrees
        mrms real,     -- Attitude phase measurement RMS error in meters
	brms real,     -- Attitude baseline length RMS error in meters
        flag int2 CHECK (flag=0 or flag=1),    -- Quality flag
	latitude real, -- Payload latitude in degrees
        longitude real, -- Payload longitude in degrees
        altitude real,   -- Payload altitude in degrees
	UNIQUE(time,us)
);
CREATE INDEX adu5pat_time_index ON adu5_pat (time);

-- ADU5 GPS velocity table
CREATE TABLE adu5_vtg (
	nbuf int8 PRIMARY KEY,
	crc int2,   -- See comment on top   
	now int8,   -- time of processing
	time int8, 
	us int8,
	course real,   -- True course 
	mcourse real,  -- Magnetic course
	vkt  real,  -- Speed in knots
	vkph real,  -- Speed in kph
	UNIQUE(time,us)
);
CREATE INDEX adu5vtg_time_index ON adu5_vtg (time);

-- ADU5 GPS satellite info table
CREATE TABLE adu5_sat (
	nbuf int8 PRIMARY KEY,
	crc int2,   -- See comment on top   
	now int8,   -- time of processing
	time int8 UNIQUE, 
	numsats int2[],      -- Number of satellites, 4 values
	prn int2[][],        -- PRN value (4 by numsats, up to 20)
	elevation int2[][],  -- Satellite elevation (4 by numsats, up to 20)
	snr int2[][],        -- Satellite signal SNR (4 by numsats, up to 20)
	flag int2[][],       -- Quality flag (4 by numsats, up to 20)
	azimuth int4[][]     -- Satellite azimuth (4 by numsats, up to 20)
);
CREATE INDEX adu5sat_time_index ON adu5_sat (time);

-- G12 GPS position and velocity table
CREATE TABLE g12_pos (
	nbuf int8 PRIMARY KEY,
	crc int2,   -- See comment on top   
	now int8,   -- time of processing
	time int8, 
	us int8,
	tod int8,      -- Time of Day in ms
	numsats int4,  -- Number of satellites
	latitude real, -- Payload latitude in degrees
        longitude real, -- Payload longitude in degrees
        altitude real,   -- Payload altitude in degrees
	course real,  -- True course 
	upv  real,  -- Vertical velocity (units?)
	vkt real,   -- Speed in knots
	pdop real,  -- PDOP value
	hdop real,  -- HDOP value
	vdop real,  -- VDOP value
	tdop real,  -- TDOP value
	UNIQUE(time,us)
);
CREATE INDEX g12pos_time_index ON g12_pos (time);

-- G12 GPS satellite info table
CREATE TABLE g12_sat (
	nbuf int8 PRIMARY KEY,
	crc int2,   -- See comment on top   
	now int8,   -- time of processing
	time int8 UNIQUE, 
	numsats int2,      -- Number of satellites
	prn int2[],        -- PRN value (numsats, up to 20)
	elevation int2[],  -- Satellite elevation (numsats, up to 20)
	snr int2[],        -- Satellite signal SNR (numsats, up to 20)
	flag int2[],       -- Quality flag (numsats, up to 20)
	azimuth int4[]     -- Satellite azimuth (numsats, up to 20)
);
CREATE INDEX g12sat_time_index ON g12_sat (time);

-- Command echo table
CREATE TABLE cmd (
	nbuf int8 PRIMARY KEY,
	crc int2,    -- See comment on top
	now int8,   -- time of processing
	time int8 UNIQUE,
	flag int2 CHECK (flag=0 or flag=1),
	bytes int2, -- Number of bytes
	cmd int2[]  -- command bytes, up to 10
);
CREATE INDEX cmd_time_index ON cmd (time);

-- Wakeup packet table
CREATE TABLE wakeup (
	nbuf int8 PRIMARY KEY,
	crc int2,    -- See comment on top
	now int8,   -- time of processing
	type int2   -- 0=LOS, 1=HIRATE, 2=COMM 1, 3=COMM 2
);

-- Header table
CREATE TABLE hd (
	nbuf int8 PRIMARY KEY,
	crc int2,   -- See comment on top   
	now int8,   -- time of processing
	time int8,
	us   int8,  -- UNIX microsecond
        ns   int8,  -- Nanosecond part of second from GPS
	evnum int8 UNIQUE, -- Event number
	surfmask int2, -- SURF mask (12-bit value)
	calib int2, -- Calibration status 
	priority int2,  -- Event priority 
	turfword int2, -- Turf upper word
	antmask int8, -- Antenna trigger mask
	trigtype int2, -- TURFIO trigger type
        trignum int4,   -- TURFIO trig number
        l3cnt   int2,  -- TURFIO L3 type1 count
        pps int8,    -- TURFIO pps number
	trigtime int8,  -- TURFIO trig time
	c3po int8,   -- C3PO number
	l1trigpat int8,  -- L1TrigPattern;
	l2trigpat int8,  -- L2TrigPattern;
	l3trigpat int4,   -- l3TrigPattern;
	wvin int2[], -- Track which waveforms are present, index is channel id
	hk int8 REFERENCES hk_raw,      -- Associated HK data nbuf
	surf int8 REFERENCES hk_surf,   -- Associated SURF HK data nbuf
	turf int8 REFERENCES turf,      -- Associated TURF rate data nbuf
	pat int8 REFERENCES adu5_pat,   -- Associated ADU5 GPS position data nbuf
	cmd int8 REFERENCES cmd,        -- Associated command echo nbuf
	UNIQUE (time,us)
);
CREATE INDEX hd_time_index ON hd (time);
CREATE INDEX hd_evnum_index ON hd (evnum);

-- Raw waveform table
CREATE TABLE wv (
	nbuf int8,  -- Don't key nbuf since multiple waveforms can come in one buffer
	crc int2,   -- See comment on top
	now int8,   -- time of processing
	evnum int8, -- Reference to header 
	id int2,    -- Channel id number (chan+9*surf) (0-80)
	chip int2,  -- Chip read out
	hbwrap int2 CHECK (hbwrap=0 or hbwrap=1), -- Hit bus wrapped
	hbstart int2,  -- Position of first hitbus bin before rotation
        hbend int2,    -- Position of last hitbus bin before rotation
	mean double precision,     -- Mean value as calculated by prioritizer
	rms double precision,      -- RMS as calculated by prioritizer
	raw int2[],    -- Raw data, full 16 bits (or less in case of encoding...), 260 values
	cal real[],     -- Calibrated and rotated data (mV)
	UNIQUE (evnum,id)
);
CREATE INDEX wv_evnum_index ON wv (evnum);

-- Fixed table with pedestal values for waveforms
CREATE TABLE wv_ped (
	time int8,   -- Time reference of pedestals
	now int8,    -- time of processing
	id int2,     -- Channel id number (chan+9*surf) (0-80)
	chip int2,   -- Chip (0-3)
	ped real[]   -- Pedestal values, 260 per chip
);
-- Populate pedestal table
-- Need some default pedestals...
COPY wv_ped (time,now,id,chip,ped) FROM '/tmp/pedValues.txt' WITH DELIMITER AS ' ';
--\copy wv_ped from '/tmp/pedValues.txt' with delimiter ' '

-- Create waveform subtraction operator (obsolete now, but nifty ;)
CREATE OR REPLACE FUNCTION wvdiff(real[],real[]) RETURNS real[] AS '
  DECLARE 
    wv1 ALIAS FOR $1;
    wv2 ALIAS FOR $2; 
    diff real[]:=''{}'';
  BEGIN
    FOR i IN 1..260 LOOP  -- right?
       diff[i]:=wv1[i]-wv2[i];
    END LOOP;

    RETURN diff;
  END;
' LANGUAGE plpgsql; 
DROP OPERATOR @-(real[],real[]) CASCADE;
CREATE OPERATOR @- (
     LEFTARG = real[],
    RIGHTARG = real[],
    PROCEDURE = wvdiff
);

-- Create function which will calibrate waveforms entries
CREATE OR REPLACE FUNCTION calwv() RETURNS trigger AS '
  DECLARE 
     calwv real[]:=''{}'';	-- Calibrated voltage
     rotwv real[]:=''{}'';	-- Rotated waveform
     pedtime int8;
     hdtime int8;	
     pedwv real[]:=''{}'';
     ir int2:=1; -- Index for rotated waveform 
  BEGIN
-- First find corresponding pedestals	
     SELECT INTO hdtime time FROM hd WHERE evnum=NEW.evnum;
     IF NOT FOUND THEN 
      -- RAISE EXCEPTION ''No header for event # %'', NEW.evnum;
      -- These will be calibrated later when header arrives
         RETURN NEW;
     END IF;

     UPDATE hd SET wvin[NEW.id+1]=1 WHERE evnum=NEW.evnum;

     SELECT INTO pedtime MAX(time) FROM wv_ped WHERE time<=hdtime;
     IF NOT FOUND THEN 
	RAISE EXCEPTION ''No pedestals before time %'',hdtime; 
--        RETURN NEW;
     END IF;
     SELECT INTO pedwv ped FROM wv_ped WHERE time=pedtime AND id=NEW.id AND chip=NEW.chip;
     IF pedwv IS NULL THEN
	RAISE EXCEPTION ''No pedestal record for time=% id=% chip=%'',pedtime,NEW.id,NEW.chip;
--	RETURN NEW;
     END IF;

-- Calibrate voltages
     FOR i IN 1..260 LOOP
	-- First handle zeros in first bin
        IF NEW.raw[i]=0 THEN
          calwv[i]:=0;
        ELSE  
  	  -- Subtract pedestal values and gain correction
	  calwv[i] := (NEW.raw[i]-pedwv[i])*1.17;  -- mV/ADC, per Gary email of 05/04/2006 
        END IF;
     END LOOP;

-- Perform rotation 
    IF NEW.hbwrap=1 THEN -- Wrapped hitbus
	FOR i IN (NEW.hbstart+2)..(NEW.hbend-1) LOOP
	   rotwv[ir]:=calwv[i];	
	   ir:=ir+1;
	END LOOP;
    ELSE
	FOR i IN (NEW.hbend+2)..259 LOOP  -- Ignore SCA=259 
	   rotwv[ir]:=calwv[i];	
	   ir:=ir+1;
	END LOOP;
        FOR i IN 2..(NEW.hbstart-1) LOOP  -- Ignore SCA=0
	   rotwv[ir]:=calwv[i];	
	   ir:=ir+1;
	END LOOP;
     END IF;
     FOR i in ir..260 LOOP  -- Fill in remaining bins with zeros
       rotwv[i]:=0;
     END LOOP;
     NEW.cal:=rotwv;

 --    EXECUTE NOTIFY to_char(NEW.id,''"wv"00'');

     RETURN NEW;
  END;
' LANGUAGE plpgsql;

CREATE TRIGGER calwv BEFORE INSERT OR UPDATE ON wv
    FOR EACH ROW EXECUTE PROCEDURE calwv();

-- Create function which will retroactively calibrate waveforms which arrive
-- before header
CREATE OR REPLACE FUNCTION retrocalwv(int2,int8,int8) RETURNS void AS '
  DECLARE 
     calwv real[]:=''{}'';	-- Calibrated voltage
     rotwv real[]:=''{}'';	-- Rotated waveform
     pedtime int8;
     wvid ALIAS FOR $1;
     wvevnum ALIAS FOR $2;	
     hdtime ALIAS FOR $3;	
     pedwv real[]:=''{}'';
     wvrec RECORD;
     ir int2:=1; -- Index for rotated waveform 
  BEGIN
-- First find corresponding pedestals	
     SELECT INTO pedtime MAX(time) FROM wv_ped WHERE time<=hdtime;
     IF NOT FOUND THEN 
        RETURN;
     END IF;

     SELECT INTO wvrec * FROM wv WHERE id=wvid AND evnum=wvevnum;

     SELECT INTO pedwv ped FROM wv_ped WHERE time=pedtime AND id=wvid AND chip=wvrec.chip;
     IF NOT FOUND THEN 
	RETURN;
     END IF;

-- Calibrate voltages
     FOR i IN 1..260 LOOP
	-- First handle zeros in first bin
        IF wvrec.raw[i]=0 THEN
          calwv[i]:=0;
        ELSE  
  	  -- Subtract pedestal values and linearity correction
	  calwv[i] := (wvrec.raw[i]-pedwv[i])*1.17;  -- mV/ADC, per Gary email of 05/04/2006 
        END IF;
     END LOOP;

-- Perform rotation 
     IF wvrec.hbwrap=1 THEN -- Wrapped hitbus
	FOR i IN (wvrec.hbstart+2)..(wvrec.hbend-1) LOOP
	   rotwv[ir]:=calwv[i];	
	   ir:=ir+1;
	END LOOP;
     ELSE
	FOR i IN (wvrec.hbend+2)..259 LOOP  -- Ignore last SCA=259
	   rotwv[ir]:=calwv[i];	
	   ir:=ir+1;
	END LOOP;
        FOR i IN 2..(wvrec.hbstart-1) LOOP  -- Ignore first SCA=0
	   rotwv[ir]:=calwv[i];	
	   ir:=ir+1;
	END LOOP;
     END IF;
     FOR i in ir..260 LOOP  -- Fill in remaining bins with zeros
       rotwv[i]:=0;
     END LOOP;
     UPDATE wv SET cal=rotwv WHERE id=wvid AND evnum=wvevnum;	

     RETURN;
  END;
' LANGUAGE plpgsql;

-- Trigger function to be run when new event header is inserted
CREATE OR REPLACE FUNCTION newhd() RETURNS trigger AS '
  DECLARE 
     wvin int2[]:=''{}'';	
     wvrec RECORD;		
  BEGIN
-- Initialize wvin variable
     FOR i IN 1..81 LOOP
        wvin[i]:=0;
     END LOOP;

-- Now check if any waveforms already arrived 
     FOR wvrec IN SELECT id FROM wv WHERE evnum=NEW.evnum LOOP
        wvin[wvrec.id+1]:=1;
	-- Now need to calibrate event retroactively
	PERFORM retrocalwv(wvrec.id,NEW.evnum,NEW.time);
     END LOOP;
     NEW.wvin:=wvin;

     RETURN NEW;
  END;
' LANGUAGE plpgsql;

CREATE TRIGGER newhd BEFORE INSERT ON hd
    FOR EACH ROW EXECUTE PROCEDURE newhd();

-- Trigger function to be run when new hk data is inserted
CREATE OR REPLACE FUNCTION newhk() RETURNS trigger AS '
  DECLARE 
     dt integer := 60;
     hdrec RECORD;
     hktime int8;
     hkus int8;
  BEGIN
-- If this is calibration event, no action
     IF NEW.code=512 OR NEW.code=768 THEN
       RETURN NEW;
     END IF;

-- If update w/o time reference change, no action
     IF TG_OP = ''UPDATE'' THEN
       IF NEW.time = OLD.time AND NEW.us = OLD.us THEN
         RETURN NEW;
       END IF;
     END IF;

     FOR hdrec IN SELECT time,us,hk FROM hd WHERE (time>(NEW.time-dt) AND time<(NEW.time+dt)) LOOP
	 SELECT INTO hktime,hkus time,us FROM hk_raw WHERE nbuf=hdrec.hk;
         IF ABS((hdrec.time+hdrec.us/1e6)-(NEW.time+NEW.us/1e6))<ABS((hdrec.time+hdrec.us/1e6)-(hktime+hkus/1e6)) OR hdrec.hk=-1 THEN
          UPDATE hd SET hk=NEW.nbuf WHERE time=hdrec.time AND us=hdrec.us;
         END IF;
     END LOOP;
     RETURN NEW;
  END;
' LANGUAGE plpgsql;

-- Trigger function to be run when new GPS position data is inserted
CREATE OR REPLACE FUNCTION newpat() RETURNS trigger AS '
  DECLARE 
     dt integer := 60;
     hdrec RECORD;
     pattime int8;
     patus int8;	
  BEGIN
-- If update w/o time reference change, no action
     IF TG_OP = ''UPDATE'' THEN
       IF NEW.time = OLD.time AND NEW.us = OLD.us THEN
         RETURN NEW;
       END IF;
     END IF;

     FOR hdrec IN SELECT time,us,pat FROM hd WHERE (time>(NEW.time-dt) AND time<(NEW.time+dt)) LOOP
	 SELECT INTO pattime,patus time,us FROM adu5_pat WHERE nbuf=hdrec.pat;
         IF ABS((hdrec.time+hdrec.us/1e6)-(NEW.time+NEW.us/1e6))<ABS((hdrec.time+hdrec.us/1e6)-(pattime+patus/1e6)) OR hdrec.pat=-1 THEN
           UPDATE hd SET pat=NEW.nbuf WHERE time=hdrec.time AND us=hdrec.us;
         END IF;
     END LOOP;
     RETURN NEW;
  END;
' LANGUAGE plpgsql;

-- Trigger function to be run when new SURF hk data is inserted
CREATE OR REPLACE FUNCTION newsurf() RETURNS trigger AS '
  DECLARE 
     dt integer := 60;
     hdrec RECORD;
     surftime int8;
     surfus int8;	
  BEGIN
-- If update w/o time reference change, no action
     IF TG_OP = ''UPDATE'' THEN
       IF NEW.time = OLD.time AND NEW.us = OLD.us THEN
         RETURN NEW;
       END IF;
     END IF;

     FOR hdrec IN SELECT time,us,surf FROM hd WHERE (time>(NEW.time-dt) AND time<(NEW.time+dt)) LOOP
         SELECT INTO surftime,surfus time,us FROM hk_surf WHERE nbuf=hdrec.surf;
         IF ABS((hdrec.time+hdrec.us/1e6)-(NEW.time+NEW.us/1e6))<ABS((hdrec.time+hdrec.us/1e6)-(surftime+surfus/1e6)) OR hdrec.surf=-1 THEN
           UPDATE hd SET surf=NEW.nbuf WHERE time=hdrec.time AND us=hdrec.us;
         END IF;
     END LOOP;
     RETURN NEW;
  END;
' LANGUAGE plpgsql;

-- Trigger function to be run when new TURF rate data is inserted
CREATE OR REPLACE FUNCTION newturf() RETURNS trigger AS '
  DECLARE 
     dt integer := 60;
     hdrec RECORD;
     turftime int8;
     turfus int8;
  BEGIN
-- If update w/o time reference change, no action
     IF TG_OP = ''UPDATE'' THEN
       IF NEW.time = OLD.time AND NEW.us = OLD.us THEN
         RETURN NEW;
       END IF;
     END IF;

     FOR hdrec IN SELECT time,us,turf FROM hd WHERE (time>(NEW.time-dt) AND time<(NEW.time+dt)) LOOP
	 SELECT INTO turftime,turfus time,us FROM turf WHERE nbuf=hdrec.turf;
         IF ABS((hdrec.time+hdrec.us/1e6)-(NEW.time+NEW.us/1e6))<ABS((hdrec.time+hdrec.us/1e6)-(turftime+turfus/1e6)) OR hdrec.turf=-1 THEN
           UPDATE hd SET turf=NEW.nbuf WHERE time=hdrec.time AND us=hdrec.us;
         END IF;
     END LOOP;
     RETURN NEW;
  END;
' LANGUAGE plpgsql;

-- Trigger function to be run when new command echo is inserted
CREATE OR REPLACE FUNCTION newcmd() RETURNS trigger AS '
  DECLARE
     hdrec RECORD;
     cmdtime int8;
  BEGIN
-- If update w/o time reference change, no action
     IF TG_OP = ''UPDATE'' THEN
       IF NEW.time = OLD.time THEN
         RETURN NEW;
       END IF;
     END IF;

     FOR hdrec IN SELECT time,cmd FROM hd WHERE (time>=NEW.time) LOOP
	 SELECT INTO cmdtime time FROM cmd WHERE nbuf=hdrec.cmd;
         IF (hdrec.time-NEW.time)<(hdrec.time-cmdtime) OR hdrec.cmd IS NULL THEN
           UPDATE hd SET cmd=NEW.nbuf WHERE time=hdrec.time;
         END IF;
     END LOOP;
     RETURN NEW;
  END;
' LANGUAGE plpgsql;

-- No trigger for now, since they slow down DB insertion (and we don't really use them)
--CREATE TRIGGER newhk AFTER INSERT OR UPDATE ON hk_raw
--    FOR EACH ROW EXECUTE PROCEDURE newhk();
--CREATE TRIGGER newpat AFTER INSERT OR UPDATE ON adu5_pat
--    FOR EACH ROW EXECUTE PROCEDURE newpat();
--CREATE TRIGGER newsurf AFTER INSERT OR UPDATE ON hk_surf
--    FOR EACH ROW EXECUTE PROCEDURE newsurf();
--CREATE TRIGGER newturf AFTER INSERT OR UPDATE ON turf
--    FOR EACH ROW EXECUTE PROCEDURE newturf();
--CREATE TRIGGER newcmd AFTER INSERT OR UPDATE ON cmd
--    FOR EACH ROW EXECUTE PROCEDURE newcmd();

-- ISSUE NOTIFICATIONS FOR ALL TABLES HERE --
CREATE RULE hk_new AS ON INSERT TO hk DO NOTIFY hk;
CREATE RULE hk_surf_new AS ON INSERT TO hk_surf DO NOTIFY hk_surf;
CREATE RULE turf_new AS ON INSERT TO turf DO NOTIFY turf;
CREATE RULE mon_new AS ON INSERT TO mon DO NOTIFY mon;
CREATE RULE adu5_pat_new AS ON INSERT TO adu5_pat DO NOTIFY adu5_pat;
CREATE RULE adu5_vtg_new AS ON INSERT TO adu5_vtg DO NOTIFY adu5_vtg;
CREATE RULE adu5_sat_new AS ON INSERT TO adu5_sat DO NOTIFY adu5_sat;
CREATE RULE g12_pos_new AS ON INSERT TO g12_pos DO NOTIFY g12_pos;
CREATE RULE g12_sat_new AS ON INSERT TO g12_sat DO NOTIFY g12_sat;
CREATE RULE hd_new AS ON INSERT TO hd DO NOTIFY hd;
CREATE RULE cmd_new AS ON INSERT TO cmd DO NOTIFY cmd;

-- Create user apache and allow read access to all tables for web access
CREATE USER apache;
GRANT SELECT ON hk_raw TO apache;
GRANT SELECT ON hk TO apache;
GRANT SELECT ON hk_surf TO apache;
GRANT SELECT ON turf TO apache;
GRANT SELECT ON mon TO apache;
GRANT SELECT ON adu5_pat TO apache;
GRANT SELECT ON adu5_sat TO apache;
GRANT SELECT ON adu5_vtg TO apache;
GRANT SELECT ON g12_pos TO apache;
GRANT SELECT ON g12_sat TO apache;
GRANT SELECT ON wv TO apache;
GRANT SELECT ON wv_ped TO apache;
GRANT SELECT ON hd TO apache;
GRANT SELECT ON cmd TO apache;
GRANT SELECT ON hk TO apache;

-- Create user gui and allow read access to all tables for viewer access
CREATE USER gui;
GRANT SELECT ON hk_raw TO gui;
GRANT SELECT ON hk TO gui;
GRANT SELECT ON hk_surf TO gui;
GRANT SELECT ON turf TO gui;
GRANT SELECT ON mon TO gui;
GRANT SELECT ON adu5_pat TO gui;
GRANT SELECT ON adu5_sat TO gui;
GRANT SELECT ON adu5_vtg TO gui;
GRANT SELECT ON g12_pos TO gui;
GRANT SELECT ON g12_sat TO gui;
GRANT SELECT ON wv TO gui;
GRANT SELECT ON wv_ped TO gui;
GRANT SELECT ON hd TO gui;
GRANT SELECT ON cmd TO gui;
GRANT SELECT ON hk TO gui;
