
-- File creates tables and trigger functions for ANITA flight database
-- TODO insert NOTIFY for user end table entries...

-- User should use 'hk' view to access housekeeping data

-- CRC entries are two byte; top byte gives wrapper CRC quality, bottom byte internal CRC quality
-- In both cases 0=failed CRC check, 1=passed CRC check
-- Additionaly for waveforms; bottom byte uses following encoding;
--          0 = both individual waveform and waveform transfer packet failed check
--          1 = waveform transfer packet ok, individual waveform failed
--          2 = waveform transfer packet failed, individual waveform ok (strange case!)
--          3 = both individual waveform and waveform transfer packet passed check

-- Raw housekeeping table
CREATE TABLE hk_cal (
	nbuf int PRIMARY KEY,
	crc int2,   -- see CRC comment at the top
	now int,   -- time of processing
	time int, 
	us int, 
	code int2 CHECK (code=512 OR code=768),
	bd1  int2[],    -- Board 1 calib data 
	bd2  int2[],    -- Board 2 calib data
	bd3  int2[],    -- Board 3 calib data
	UNIQUE (time,us,code)
) WITHOUT OIDS;
-- These are default AVZ and CAL calibrations from EM flight.... 
\copy hk_cal (nbuf,time,us,code,crc,bd1,bd2,bd3) FROM '/tmp/hkdValues.txt' WITH DELIMITER as ' '

-- Calibrated housekeeping table
CREATE TABLE hk (
	nbuf int PRIMARY KEY,
	crc int2,   -- see CRC comment at the top
	time int, 
	us int, 
	now int, -- time of processing
	code int2 CHECK (code=256),

	cal int REFERENCES hk_cal,   -- References nbuf of CAL code hk packet  
	avz int REFERENCES hk_cal,   -- References nbuf of AVZ code hk packet  
	bd1  int2[],    -- Board 1 raw data 
	bd2  int2[],    -- Board 2 raw data
	bd3  int2[],    -- Board 3 raw data
	calb1 real[],    -- Board 1 voltage 
	calb2 real[],    -- Board 2 voltage
	calb3 real[],    -- Board 3 voltage
	accx real[],  -- Accelerometer, x-axes
	accy real[],  -- Accelerometer, y-axes
	accz real[],  -- Accelerometer, z-axes
	acct real[],  -- Accelerometer, temperatures
	ssx real[],    -- Sunsensor X-coordinates
	ssy real[],    -- Sunsensor Y-coordinates
	ssi real[],    -- Sunsensor intensity, average sum of x1 + x2 and y1 +y2
	ssflag int2[], -- Sunsensore reading quality, 1 good, 0 bad
        ssel real[],   -- Sun elevation
        ssaz real[],   -- Sun azimuth relative to ANITA x-axis
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
	p5vip real,    ---peng added
	it real[], -- Internal temperatures in Celsius
	et real[],  -- External temperatures in Celsius
	sbst1 int4,     -- SBS CPU temperatures
        sbst2 int4,  
	core1 int4,     -- SBS CPU ore temperatures
        core2 int4,  
	sbst5 int4,    --peng 
        sbst6 int4,    --peng
	magx  real,   -- Magnetometer info 
	magy  real, 
	magz  real,
	UNIQUE(time,us)
) WITHOUT OIDS;
CREATE INDEX hk_now_index ON hk (now);	

-- Raw ss housekeeping table
CREATE TABLE sshk_cal (
	nbuf int PRIMARY KEY,
	crc int2,   -- see CRC comment at the top
	now int,   -- time of processing
	time int, 
	us int, 
	code int2 CHECK (code=512 OR code=768),
	bd1  int2[],    -- Board 1 calib data 
	--bd2  int2[],    -- Board 2 calib data
	--bd3  int2[],    -- Board 3 calib data
	UNIQUE (time,us,code)
) WITHOUT OIDS;
-- These are default AVZ and CAL calibrations from EM flight.... 
\copy sshk_cal (nbuf,time,us,code,crc,bd1) FROM '/tmp/sshkdValues.txt' WITH DELIMITER as ' '
CREATE TABLE SShk (
	nbuf int PRIMARY KEY,
	crc int2,   -- see CRC comment at the top
	time int, 
	us int, 
	now int, -- time of processing
	code int2 CHECK (code=256),---peng

	cal int REFERENCES sshk_cal,   -- References nbuf of CAL code hk packet  
        avz int REFERENCES sshk_cal,   -- References nbuf of AVZ code hk packet  
	bd1 int2[],    -- Board 1 raw data 
	calb1 real[],    -- Board 1 voltage 

	ssx real[],    -- Sunsensor X-coordinates
	ssy real[],    -- Sunsensor Y-coordinates
	ssi real[],    -- Sunsensor intensity, average sum of x1 + x2 and y1 +y2
	ssflag int2[], -- Sunsensore reading quality, 1 good, 0 bad
        ssel real[],   -- Sun elevation
        ssaz real[],   -- Sun azimuth relative to ANITA x-axis
	sst real[],    -- Sunsensor temperatures
	UNIQUE(time,us)
) WITHOUT OIDS;
CREATE INDEX SShk_now_index ON SShk (now);	

-- Create function for sunsensor calibration
CREATE OR REPLACE FUNCTION calss(integer,real,real) RETURNS RECORD AS '
  DECLARE
    n ALIAS for $1;
    x ALIAS for $2;
    y ALIAS for $3;	
    gainX double precision[]:=''{5.0288,4.8515,5.0599,5.0288}'';
    gainY double precision[]:=''{5.0,5.0,5.0,5.0}'';
    offsetX double precision[]:=''{0.0800,-0.32940,0.05541,-0.23773}'';
    offsetY double precision[]:=''{-0.1572,-0.17477,-0.08458,-0.50356}'';
    sep double precision[]:=''{3.704391,3.618574,3.512025,3.554451}'';
    cosg double precision[]:=''{0.38912395,0.37945616,0.387515586,0.390731128}'';
    sing double precision[]:=''{0.921185406,0.925209718,0.921863152,0.920504853}'';
    sens_az double precision[]:=''{45,-45,-135,135}'';
    x0 double precision;
    y0 double precision;	
    pos double precision[]=''{}'';
    az_meas real;
    el real;   
    az real;
    retval RECORD;
  BEGIN
    x0 := x*gainX[n]-offsetX[n];
    y0 :=-y*gainY[n]-offsetY[n];

    -- Sun is now at (-x0,-y0,sep[n]) in frame of sensor
    -- Rotate to frame with z pointing up
    pos[1]:=-x0*cosg[n]-sep[n]*sing[n];
    pos[2]:=-y0;
    pos[3]:=-x0*sing[n]+sep[n]*cosg[n];

    az_meas := atan2(pos[2],-pos[1])*57.29577951;
    el := atan2(pos[3],sqrt(pos[1]*pos[1]+pos[2]*pos[2]))*57.29577951;

    -- Convert to azimuth relative to ADU5 fore 
    az := az_meas + sens_az[n] + 360;
    WHILE az>360 LOOP
      az := az - 360;
    END LOOP;

    WHILE az<0 LOOP
      az := az + 360;
    END LOOP;	

    SELECT el,az INTO retval;	
    RETURN retval;
  END;
' LANGUAGE plpgsql;

-- Create function for ss sunsensor calibration
CREATE OR REPLACE FUNCTION calss2(integer,real,real) RETURNS RECORD AS '
  DECLARE
    n ALIAS for $1;
    x ALIAS for $2;
    y ALIAS for $3;	
    gainX double precision[]:=''{5.0288,4.8515,5.0599,5.0288}'';
    gainY double precision[]:=''{5.0,5.0,5.0,5.0}'';
    offsetX double precision[]:=''{0.0800,-0.32940,0.05541,-0.23773}'';
    offsetY double precision[]:=''{-0.1572,-0.17477,-0.08458,-0.50356}'';
    sep double precision[]:=''{3.704391,3.618574,3.512025,3.554451}'';
    cosg double precision[]:=''{0.38912395,0.37945616,0.387515586,0.390731128}'';
    sing double precision[]:=''{0.921185406,0.925209718,0.921863152,0.920504853}'';
    sens_az double precision[]:=''{0,0,270,90}'';
    --sens_az double precision[]:=''{45,-45,-135,135}'';
    x0 double precision;
    y0 double precision;	
    pos double precision[]=''{}'';
    az_meas real;
    el real;   
    az real;
    retval RECORD;
  BEGIN
    x0 := x*gainX[n]-offsetX[n];
    y0 :=-y*gainY[n]-offsetY[n];

    -- Sun is now at (-x0,-y0,sep[n]) in frame of sensor
    -- Rotate to frame with z pointing up
    pos[1]:=-x0*cosg[n]-sep[n]*sing[n];
    pos[2]:=-y0;
    pos[3]:=-x0*sing[n]+sep[n]*cosg[n];

    az_meas := atan2(pos[2],-pos[1])*57.29577951;
    el := atan2(pos[3],sqrt(pos[1]*pos[1]+pos[2]*pos[2]))*57.29577951;

    -- Convert to azimuth relative to ADU5 fore 
    az := az_meas + sens_az[n] + 360;
    WHILE az>360 LOOP
      az := az - 360;
    END LOOP;

    WHILE az<0 LOOP
      az := az + 360;
    END LOOP;	

    SELECT el,az INTO retval;	
    RETURN retval;
  END;
' LANGUAGE plpgsql;

-- Create function which will calculate voltages for HK Acromag entries
-- and calibrate physical values
CREATE OR REPLACE FUNCTION calhk() RETURNS trigger AS '
  DECLARE 
    caltime int;
    avztime int;
    hkcal RECORD;
    hkavz RECORD;
    slope real;    -- Temp variable
    sscal RECORD;	
    cal_c int;
    avz_c int;
  BEGIN
  -- Find last CAL packet
     SELECT INTO caltime MAX(time) FROM hk_cal WHERE (code=768 AND (crc=257 OR crc=1) AND (time<NEW.time OR (time=NEW.time AND us<NEW.us)));
     IF NOT FOUND THEN
    	RAISE WARNING ''No CAL hk packet before time %d'', NEW.time;
	RETURN NEW;
     ELSE 
        SELECT INTO hkcal nbuf,bd1,bd2,bd3 FROM hk_cal WHERE time=caltime AND code=768;
        NEW.cal:=hkcal.nbuf;
     END IF;

  -- Find last AVZ packet
     SELECT INTO avztime MAX(time) FROM hk_cal WHERE (code=512 AND (crc=257 OR crc=1) AND (time<NEW.time OR (time=NEW.time AND us<NEW.us)));
     IF NOT FOUND THEN
    	RAISE WARNING ''No AVZ hk packet before time %d'', NEW.time;
	RETURN NEW;
     ELSE 
        SELECT INTO hkavz nbuf,bd1,bd2,bd3 FROM hk_cal WHERE time=avztime AND code=512;
        NEW.avz:=hkavz.nbuf;
     END IF;

     NEW.calb1:=''{}'';
     NEW.calb2:=''{}'';
     NEW.calb3:=''{}'';
     cal_c=4055;    --constant
     avz_c=2048;    --constant

     FOR i IN 1..40 LOOP
	  slope := 4.9 / (cal_c-avz_c);
	  NEW.calb1[i] := slope*(NEW.bd1[i] - avz_c);

          slope := 4.9 / (cal_c-avz_c);
	  NEW.calb2[i] := slope*(NEW.bd2[i] - avz_c);

          slope := 4.9 / (cal_c-avz_c);
	  NEW.calb3[i] := slope*(NEW.bd3[i] - avz_c);

	  --slope := 4.9 / (cal_c[i]-avz_c[i]);
	  --NEW.calb1[i] := slope*(NEW.bd1[i] - avz_c[i]);

          --slope := 4.9 / (cal_c[i]-avz_c[i]);
	  --NEW.calb2[i] := slope*(NEW.bd2[i] - avz_c[i]);

          --slope := 4.9 / (cal_c[i]-avz_c[i]);
	  --NEW.calb3[i] := slope*(NEW.bd3[i] - avz_c[i]);

     END LOOP;

-- Create physical values based on documentation
     IF NEW.calb2[7]<>0 THEN
       NEW.accx:=''{}'';
       NEW.accy:=''{}'';
       NEW.accz:=''{}'';
       NEW.acct:=''{}'';
       NEW.accx[1]:=((2.5*NEW.calb2[31])/NEW.calb2[7]-2.493)/0.830;	
       NEW.accy[1]:=((2.5*NEW.calb2[11])/NEW.calb2[7]-2.512)/0.839;	
       NEW.accz[1]:=((2.5*NEW.calb2[30])/NEW.calb2[7]-2.506)/0.842;	
       NEW.accx[2]:=((2.5*NEW.calb2[29])/NEW.calb2[7]-2.523)/0.827;	
       NEW.accy[2]:=((2.5*NEW.calb2[9])/NEW.calb2[7]-2.523)/0.844;	
       NEW.accz[2]:=((2.5*NEW.calb2[28])/NEW.calb2[7]-2.523)/0.843;	
       NEW.acct[1]:=44.4*((2.5*NEW.calb2[10])/NEW.calb2[7]-1.375);	
       NEW.acct[2]:=44.4*((2.5*NEW.calb2[8])/NEW.calb2[7]-1.375);	
     END IF;

     NEW.ssx:=''{}'';
     NEW.ssy:=''{}'';
     NEW.ssi:=''{}'';
     NEW.ssflag:=''{}'';
     NEW.ssel:=''{}'';
     NEW.ssaz:=''{}'';
     NEW.sst:=''{}'';
     IF (NEW.calb1[1]+NEW.calb1[21])<>0 THEN
       NEW.ssx[1]:=(NEW.calb1[1]-NEW.calb1[21])/(NEW.calb1[1]+NEW.calb1[21]);
     ELSE
       NEW.ssx[1]:=-2;
     END IF;
     IF (NEW.calb1[2]+NEW.calb1[22])<>0 THEN
       NEW.ssy[1]:=(NEW.calb1[2]-NEW.calb1[22])/(NEW.calb1[2]+NEW.calb1[22]);
     ELSE
       NEW.ssy[1]:=-2;
     END IF;
     IF NEW.calb1[2]>0.25 OR NEW.calb1[22]>0.25 THEN
	NEW.ssflag[1]:=1;
     ELSE
        NEW.ssflag[1]:=0;
     END IF;	
     NEW.ssi[1]:=((NEW.calb1[1]+NEW.calb1[21])+(NEW.calb1[2]+NEW.calb1[22]))/2;
     NEW.sst[1]:=NEW.calb1[3]*100-273.15;

     IF (NEW.calb1[23]+NEW.calb1[4])<>0 THEN
       NEW.ssx[2]:=(NEW.calb1[23]-NEW.calb1[4])/(NEW.calb1[23]+NEW.calb1[4]);
     ELSE
       NEW.ssx[2]:=-2;
     END IF;
     IF (NEW.calb1[24]+NEW.calb1[5])<>0 THEN
       NEW.ssy[2]:=(NEW.calb1[24]-NEW.calb1[5])/(NEW.calb1[24]+NEW.calb1[5]);
     ELSE
       NEW.ssy[2]:=-2;
     END IF;
     IF NEW.calb1[24]>0.25 OR NEW.calb1[5]>0.25 THEN
	NEW.ssflag[2]:=1;
     ELSE
        NEW.ssflag[2]:=0;
     END IF;		
     NEW.ssi[2]:=((NEW.calb1[23]+NEW.calb1[4])+(NEW.calb1[24]+NEW.calb1[5]))/2;
     NEW.sst[2]:=NEW.calb1[25]*100-273.15;

     IF (NEW.calb1[27]+NEW.calb1[8])<>0 THEN
       NEW.ssx[3]:=(NEW.calb1[27]-NEW.calb1[8])/(NEW.calb1[27]+NEW.calb1[8]);
     ELSE
       NEW.ssx[3]:=-2;
     END IF;
     IF (NEW.calb1[28]+NEW.calb1[9])<>0 THEN
       NEW.ssy[3]:=(NEW.calb1[28]-NEW.calb1[9])/(NEW.calb1[28]+NEW.calb1[9]);
     ELSE
       NEW.ssy[3]:=-2;
     END IF;	
     IF NEW.calb1[28]>0.25 OR NEW.calb1[9]>0.25 THEN	
	NEW.ssflag[3]:=1;
     ELSE
        NEW.ssflag[3]:=0;
     END IF;	
     NEW.ssi[3]:=((NEW.calb1[27]+NEW.calb1[8])+(NEW.calb1[28]+NEW.calb1[9]))/2;
     NEW.sst[3]:=NEW.calb1[29]*100-273.15;

     IF (NEW.calb1[30]+NEW.calb1[10])<>0 THEN
       NEW.ssx[4]:=(NEW.calb1[30]-NEW.calb1[10])/(NEW.calb1[30]+NEW.calb1[10]);
     ELSE
       NEW.ssx[4]:=-2;
     END IF;
     IF (NEW.calb1[11]+NEW.calb1[31])<>0 THEN
       NEW.ssy[4]:=(NEW.calb1[11]-NEW.calb1[31])/(NEW.calb1[11]+NEW.calb1[31]);
     ELSE
       NEW.ssy[4]:=-2;
     END IF;	
     IF NEW.calb1[11]>0.25 OR NEW.calb1[31]>0.25 THEN
	NEW.ssflag[4]:=1;
     ELSE
        NEW.ssflag[4]:=0;
     END IF;	
     NEW.ssi[4]:=((NEW.calb1[30]+NEW.calb1[10])+(NEW.calb1[11]+NEW.calb1[31]))/2;
     NEW.sst[4]:=NEW.calb1[12]*100-273.15;

     FOR i IN 1..4 LOOP 
        SELECT INTO sscal el,az FROM calss(i,NEW.ssx[i],NEW.ssy[i]) AS (el real,az real);
	NEW.ssel[i]:=sscal.el;
	NEW.ssaz[i]:=sscal.az;
     END LOOP;

     NEW.pressh:=(NEW.calb2[32]-0.25)*3.75;  
     NEW.pressl:=0.00215181+4.0062*NEW.calb2[12]-0.0027642*NEW.calb2[12]*NEW.calb2[12];
     NEW.p1_5v:=     NEW.calb2[2];
     NEW.p3_3v:=     NEW.calb2[6]*2;
     NEW.p5v:= 	     NEW.calb2[7]*2;
     NEW.p5sbv:=     NEW.calb2[3]*2;
     NEW.p12v:=      NEW.calb2[5]*4;
     NEW.p24v:=      NEW.calb2[37]*10.1377;
     NEW.ppvv:=      NEW.calb2[38]*19.25;
     NEW.n12v:=      NEW.calb2[4]*-3.0075;
     NEW.iprf1v:=    NEW.calb2[34]*4;
     NEW.iprf2v:=    NEW.calb2[35]*2;
     NEW.p1_5i:=     NEW.calb2[21]*0.8;
     NEW.p3_3i:=     NEW.calb2[25]*60;
     NEW.p5i:= 	     NEW.calb2[26]*20;
     NEW.p5sbi:=     NEW.calb2[22]*8;
     NEW.p12i:=      NEW.calb2[24]*0.8;
     NEW.p24i:=      NEW.calb2[17]*30;
     NEW.ppvi:=      NEW.calb2[18]*20;
     NEW.n5i:= 	     NEW.calb2[27]*0.8;--this value does not exit in anita3
     NEW.n12i:=      NEW.calb2[23]*2;
     NEW.iprf2i:=    NEW.calb2[14]*8;	
     NEW.bati:=      NEW.calb2[16]*30;
     NEW.p5vip:=      NEW.calb2[36]*2;

     NEW.it:=''{}'';
     NEW.it[1]:= 	    NEW.calb3[1]*100-273.15;
     NEW.it[2]:= 	    NEW.calb3[21]*100-273.15;
     NEW.it[3]:= 	    NEW.calb3[2]*100-273.15;
     NEW.it[4]:= 	    NEW.calb3[22]*100-273.15;
     NEW.it[5]:= 	    NEW.calb3[3]*100-273.15;
     NEW.it[6]:= 	    NEW.calb3[23]*100-273.15;
     NEW.it[7]:= 	    NEW.calb3[4]*100-273.15;
     NEW.it[8]:= 	    NEW.calb3[24]*100-273.15;
     NEW.it[9]:= 	    NEW.calb3[5]*100-273.15;
     NEW.it[10]:= 	    NEW.calb3[25]*100-273.15;
     NEW.it[11]:= 	    NEW.calb3[6]*100-273.15;
     NEW.it[12]:= 	    NEW.calb3[26]*100-273.15;
     NEW.it[13]:= 	    NEW.calb3[7]*100-273.15;
     NEW.it[14]:= 	    NEW.calb3[27]*100-273.15;
     NEW.it[15]:= 	    NEW.calb3[8]*100-273.15;
     NEW.it[16]:= 	    NEW.calb3[28]*100-273.15;
     NEW.it[17]:= 	    NEW.calb3[9]*100-273.15;
     NEW.it[18]:= 	    NEW.calb3[29]*100-273.15;   
     NEW.it[19]:= 	    NEW.calb3[10]*100-273.15;   
     NEW.it[20]:= 	    NEW.calb3[30]*100-273.15;
     NEW.it[21]:= 	    NEW.calb3[11]*100-273.15;
     NEW.it[22]:= 	    NEW.calb3[31]*100-273.15;
     NEW.it[23]:= 	    NEW.calb3[12]*100-273.15;
     NEW.it[24]:= 	    NEW.calb3[32]*100-273.15;
     NEW.it[25]:= 	    NEW.calb3[13]*100-273.15;

     NEW.et:=''{}'';
     NEW.et[1]:= 	    NEW.calb3[33]*100-273.15;
     NEW.et[2]:= 	    NEW.calb3[14]*100-273.15;
     NEW.et[3]:= 	    NEW.calb3[34]*100-273.15;
     NEW.et[4]:= 	    NEW.calb3[15]*100-273.15;
     NEW.et[5]:= 	    NEW.calb3[35]*100-273.15;
     NEW.et[6]:= 	    NEW.calb3[16]*100-273.15;
     NEW.et[7]:= 	    NEW.calb3[36]*100-273.15;
     NEW.et[8]:= 	    NEW.calb3[17]*100-273.15;
     NEW.et[9]:= 	    NEW.calb3[37]*100-273.15;
     NEW.et[10]:= 	    NEW.calb3[18]*100-273.15;
     NEW.et[11]:= 	    NEW.calb3[38]*100-273.15;
     NEW.et[12]:= 	    NEW.calb3[19]*100-273.15;
     NEW.et[13]:= 	    NEW.calb3[39]*100-273.15;
     NEW.et[14]:= 	    NEW.calb3[20]*100-273.15;   
     NEW.et[15]:= 	    NEW.calb3[40]*100-273.15;
     NEW.et[16]:= 	    NEW.calb1[40]*100-273.15;
     NEW.et[17]:= 	    NEW.calb1[20]*100-273.15;
     NEW.et[18]:= 	    NEW.calb1[39]*100-273.15;
     NEW.et[19]:= 	    NEW.calb1[19]*100-273.15;
     NEW.et[20]:= 	    NEW.calb1[38]*100-273.15;
     NEW.et[21]:= 	    NEW.calb1[18]*100-273.15;
     NEW.et[22]:= 	    NEW.calb1[37]*100-273.15;
     NEW.et[23]:= 	    NEW.calb1[17]*100-273.15;
     NEW.et[24]:= 	    NEW.calb1[26]*100-273.15;   
     NEW.et[25]:= 	    NEW.calb1[16]*100-273.15;	

     RETURN NEW;
  END;
' LANGUAGE plpgsql;
--peng
-- Create function which will calculate voltages for SSHK Acromag entries
-- and calibrate physical values
CREATE OR REPLACE FUNCTION calSShk() RETURNS trigger AS '
  DECLARE 
    caltime int;
    avztime int;
    hkcal RECORD;
    hkavz RECORD;
    slope real;    -- Temp variable
    sscal RECORD;	
    cal_c int;
    avz_c int;
  BEGIN
  -- Find last CAL packet
     SELECT INTO caltime MAX(time) FROM sshk_cal WHERE (code=768 AND (crc=257 OR crc=1) AND (time<NEW.time OR (time=NEW.time AND us<NEW.us)));
     IF NOT FOUND THEN
    	RAISE WARNING ''No CAL hk packet before time %d'', NEW.time;
	RETURN NEW;
     ELSE 
        SELECT INTO hkcal nbuf,bd1 FROM sshk_cal WHERE time=caltime AND code=768;
        NEW.cal:=hkcal.nbuf;
     END IF;

  -- Find last AVZ packet
     SELECT INTO avztime MAX(time) FROM sshk_cal WHERE (code=512 AND (crc=257 OR crc=1) AND (time<NEW.time OR (time=NEW.time AND us<NEW.us)));
     IF NOT FOUND THEN
    	RAISE WARNING ''No AVZ hk packet before time %d'', NEW.time;
	RETURN NEW;
     ELSE 
        SELECT INTO hkavz nbuf,bd1 FROM sshk_cal WHERE time=avztime AND code=512;
        NEW.avz:=hkavz.nbuf;
     END IF;
     cal_c=4055;    --constant
     avz_c=2048;    --constant

     NEW.calb1:=''{}'';
     FOR i IN 1..40 LOOP
	  slope := 4.9 / (cal_c-avz_c);
	  NEW.calb1[i] := slope*(NEW.bd1[i] - avz_c);
	  --slope := 4.9 / (cal_c[i]-avz_c[i]);
	  --NEW.calb1[i] := slope*(NEW.bd1[i] - avz_c[i]);
     END LOOP;


     NEW.ssx:=''{}'';
     NEW.ssy:=''{}'';
     NEW.ssi:=''{}'';
     NEW.ssflag:=''{}'';
     NEW.ssel:=''{}'';
     NEW.ssaz:=''{}'';
     NEW.sst:=''{}'';
     IF (NEW.calb1[1]+NEW.calb1[21])<>0 THEN
       NEW.ssx[1]:=(NEW.calb1[1]-NEW.calb1[21])/(NEW.calb1[1]+NEW.calb1[21]);
     ELSE
       NEW.ssx[1]:=-2;
     END IF;
     IF (NEW.calb1[2]+NEW.calb1[22])<>0 THEN
       NEW.ssy[1]:=(NEW.calb1[2]-NEW.calb1[22])/(NEW.calb1[2]+NEW.calb1[22]);
     ELSE
       NEW.ssy[1]:=-2;
     END IF;
     IF NEW.calb1[2]>0.25 OR NEW.calb1[22]>0.25 THEN
	NEW.ssflag[1]:=1;
     ELSE
        NEW.ssflag[1]:=0;
     END IF;	
     NEW.ssi[1]:=((NEW.calb1[1]+NEW.calb1[21])+(NEW.calb1[2]+NEW.calb1[22]))/2;
     NEW.sst[1]:=NEW.calb1[3]*100-273.15;

     IF (NEW.calb1[23]+NEW.calb1[4])<>0 THEN
       NEW.ssx[2]:=(NEW.calb1[23]-NEW.calb1[4])/(NEW.calb1[23]+NEW.calb1[4]);
     ELSE
       NEW.ssx[2]:=-2;
     END IF;
     IF (NEW.calb1[24]+NEW.calb1[5])<>0 THEN
       NEW.ssy[2]:=(NEW.calb1[24]-NEW.calb1[5])/(NEW.calb1[24]+NEW.calb1[5]);
     ELSE
       NEW.ssy[2]:=-2;
     END IF;
     IF NEW.calb1[24]>0.25 OR NEW.calb1[5]>0.25 THEN
	NEW.ssflag[2]:=1;
     ELSE
        NEW.ssflag[2]:=0;
     END IF;		
     NEW.ssi[2]:=((NEW.calb1[23]+NEW.calb1[4])+(NEW.calb1[24]+NEW.calb1[5]))/2;
     NEW.sst[2]:=NEW.calb1[25]*100-273.15;

     IF (NEW.calb1[27]+NEW.calb1[8])<>0 THEN
       NEW.ssx[3]:=(NEW.calb1[27]-NEW.calb1[8])/(NEW.calb1[27]+NEW.calb1[8]);
     ELSE
       NEW.ssx[3]:=-2;
     END IF;
     IF (NEW.calb1[28]+NEW.calb1[9])<>0 THEN
       NEW.ssy[3]:=(NEW.calb1[28]-NEW.calb1[9])/(NEW.calb1[28]+NEW.calb1[9]);
     ELSE
       NEW.ssy[3]:=-2;
     END IF;	
     IF NEW.calb1[28]>0.25 OR NEW.calb1[9]>0.25 THEN	
	NEW.ssflag[3]:=1;
     ELSE
        NEW.ssflag[3]:=0;
     END IF;	
     NEW.ssi[3]:=((NEW.calb1[27]+NEW.calb1[8])+(NEW.calb1[28]+NEW.calb1[9]))/2;
     NEW.sst[3]:=NEW.calb1[29]*100-273.15;

     IF (NEW.calb1[30]+NEW.calb1[10])<>0 THEN
       NEW.ssx[4]:=(NEW.calb1[30]-NEW.calb1[10])/(NEW.calb1[30]+NEW.calb1[10]);
     ELSE
       NEW.ssx[4]:=-2;
     END IF;
     IF (NEW.calb1[11]+NEW.calb1[31])<>0 THEN
       NEW.ssy[4]:=(NEW.calb1[11]-NEW.calb1[31])/(NEW.calb1[11]+NEW.calb1[31]);
     ELSE
       NEW.ssy[4]:=-2;
     END IF;	
     IF NEW.calb1[11]>0.25 OR NEW.calb1[31]>0.25 THEN
	NEW.ssflag[4]:=1;
     ELSE
        NEW.ssflag[4]:=0;
     END IF;	
     NEW.ssi[4]:=((NEW.calb1[30]+NEW.calb1[10])+(NEW.calb1[11]+NEW.calb1[31]))/2;
     NEW.sst[4]:=NEW.calb1[12]*100-273.15;

     FOR i IN 1..4 LOOP 
        SELECT INTO sscal el,az FROM calss2(i,NEW.ssx[i],NEW.ssy[i]) AS (el real,az real);
	NEW.ssel[i]:=sscal.el;
	NEW.ssaz[i]:=sscal.az;
     END LOOP;
     RETURN NEW;
  END;
' LANGUAGE plpgsql;
--peng


CREATE TRIGGER calhk BEFORE INSERT OR UPDATE ON hk
    FOR EACH ROW EXECUTE PROCEDURE calhk();
CREATE RULE hkcal_data AS ON INSERT 
  TO hk WHERE (NEW.code=768 OR NEW.code=512)
  DO INSTEAD INSERT INTO hk_cal VALUES 
	(NEW.nbuf,NEW.crc,NEW.now,NEW.time,NEW.us,NEW.code,
	NEW.bd1,NEW.bd2,NEW.bd3);
--peng
CREATE TRIGGER calSShk BEFORE INSERT OR UPDATE ON SShk
    FOR EACH ROW EXECUTE PROCEDURE calSShk();
CREATE RULE SShkcal_data AS ON INSERT 
  TO SShk WHERE (NEW.code=768 OR NEW.code=512)
  DO INSTEAD INSERT INTO sshk_cal VALUES 
	(NEW.nbuf,NEW.crc,NEW.now,NEW.time,NEW.us,NEW.code,
	NEW.bd1);

-- SURF housekeeping table
CREATE TABLE hk_surf (
	nbuf int PRIMARY KEY,
	crc int2,   -- See comment on top
	now int,   -- time of processing
	time int, 
	us int, 
	global int4, -- Global threshold
	error  int4, -- Error flag
	scalergoals int4[], -- Scaler target rate per band in kHz
--	nadirgoals int4[],  -- Scaler target rate for nadir antennas
 	upper int4[], -- Upper words, one per SURF
 	scaler int4[][], -- Scaler values in kHz, 32 per SURF
 	thresh int4[][], -- Threshold ADC values, 32 per SURF
 	threshset int4[][], -- Thershold ADC set values, 32 per SURF
 	rfpow  int4[][], -- RF power values in ADC values, 8 per SURF	
 	l1scaler  int4[][], -- l1scaler	
 	surfmask int4[], -- Surf RF trigger band mask (32-bit fields), one per SURF
 	UNIQUE (time,us)
 ) WITHOUT OIDS;
 CREATE INDEX hk_surf_now_index ON hk_surf (now);

 -- TURF rates table
CREATE TABLE turf (
	nbuf int PRIMARY KEY,
	crc int2,   -- See comment on top
	now int,   -- time of processing
	time int, 
	deadtime int,
        l1trigmask int,	--l1TrigMask
	l1trigmaskh int,--l1TrigMaskH
	phitrigmask int,
	phitrigmaskh int,
	l1 int[],   -- Level 1 rate (16 sectors)
	l1h int[],   -- Level 1 rate (16 sectors)
	l3 int2[],   -- Level 3 rate (16 sectors)
	l3h int2[],   -- Level 3 rate (16 sectors)
	UNIQUE (time)
) WITHOUT OIDS;
CREATE INDEX turf_now_index ON turf (now);

 -- RTLSDR pow spectrum table
CREATE TABLE rtlsdr (
	nbuf int PRIMARY KEY,
	crc int2,   -- See comment on top
	now int,   -- time of processing
	nfreq int,
	startfreq int,
	freqstep int,
	unixtimestart int,
	scantime int2,
	gain int2,
	specturm char[],
	rtlnum char,
	UNIQUE (unixtimestart)
) WITHOUT OIDS;
CREATE INDEX rtlsdr_now_index ON rtlsdr (now);
 -- TUFF status table
CREATE TABLE tuff_status (
	nbuf int PRIMARY KEY,
	crc int2,   -- See comment on top
	now int,   -- time of processing
	time int, 
	notchsettime int,
	startsectors char[],
	endsectors char[],
	temperatures char[],
	UNIQUE (time)
) WITHOUT OIDS;
CREATE INDEX tuff_status_now_index ON tuff_status (now);
 -- TUFF cmd table
CREATE TABLE tuff_cmd (
	nbuf int PRIMARY KEY,
	crc int2,   -- See comment on top
	now int,   -- time of processing
	requestedtime int,
	enactedtime int,
	cmd int2,
	irfcm char,
	tuffstack char,
	UNIQUE (requestedtime)
) WITHOUT OIDS;
CREATE INDEX tuff_cmd_now_index ON tuff_cmd (now);

-- Disk space and queue monitor table
CREATE TABLE mon (
	nbuf int PRIMARY KEY,
	crc int2,   -- See comment on top
	now int,   -- time of processing
	time int UNIQUE, 
	disk int4[],  -- Disk space in MB (8 disks) (4th entry divided by 16)
	blade text, -- Label of blade disk
	usbint text, -- Label of internal USB disk
	usbext text, -- Label of external USB disk
	linkev int4[], -- Events in queue (10 priority queues)
	linkcmdlos int4, -- Command echos in queue
	linkcmdsip int4, -- Command echos in queue
	linkgps int4, -- GPS packets in queue
	linkhk  int4, -- Housekeeping links in queue
	linkmon int4, -- Monitor links in queue
	linkhd  int4, -- Header links in queue
	linksurf int4, -- SURF hk links in queue
	linkturf int4, -- TURF rate links in queue
	linkped int4 -- Pedestal links in queue 
) WITHOUT OIDS;	
CREATE INDEX mon_now_index ON mon (now);

-- ADU5 GPS position table
CREATE TABLE adu5_pat (
	gpstype int, -- ADU5A=2, ADU5B=4
	nbuf int PRIMARY KEY,
	crc int2,   -- See comment on top   
	now int,   -- time of processing
	code int,  -- packet code contains adu5 unit #
	time int, 
	us int,
	tod int,        -- Time of Day in ms
	heading real,  -- Payload heading in degrees
        pitch real,    -- Payload pitch in degrees
        roll real,     -- Payload roll in degrees
        mrms real,     -- Attitude phase measurement RMS error in meters
	brms real,     -- Attitude baseline length RMS error in meters
        flag int2 CHECK (flag=0 or flag=1),    -- Quality flag
	latitude real, -- Payload latitude in degrees
        longitude real, -- Payload longitude in degrees
        altitude real,   -- Payload altitude in meters
	UNIQUE(time,us,code)
) WITHOUT OIDS;
CREATE INDEX adu5pat_now_index ON adu5_pat (now);

-- ADU5 GPS velocity table
CREATE TABLE adu5_vtg (
	gpstype int, -- ADU5A=2, ADU5B=4
	nbuf int PRIMARY KEY,
	crc int2,   -- See comment on top   
	now int,   -- time of processing
	code int,  -- packet code contains adu5 unit #
	time int, 
	us int,
	course real,   -- True course 
	mcourse real,  -- Magnetic course
	vkt  real,  -- Speed in knots
	vkph real,  -- Speed in kph
	UNIQUE(time,us,code)
) WITHOUT OIDS;
CREATE INDEX adu5vtg_now_index ON adu5_vtg (now);

-- ADU5 GPS satellite info table
CREATE TABLE adu5_sat (
	gpstype int, -- ADU5A=2, ADU5B=4
	nbuf int PRIMARY KEY,
	crc int2,   -- See comment on top   
	now int,   -- time of processing
	code int,  -- packet code contains adu5 unit #
	time int, 
	numsats int2[],      -- Number of satellites, 4 values
	prn int2[][],        -- PRN value (4 by numsats, up to 20)
	elevation int2[][],  -- Satellite elevation (4 by numsats, up to 20)
	snr int2[][],        -- Satellite signal SNR (4 by numsats, up to 20)
	flag int2[][],       -- Quality flag (4 by numsats, up to 20)
	azimuth int4[][],     -- Satellite azimuth (4 by numsats, up to 20)
	UNIQUE(time,code)
) WITHOUT OIDS;
CREATE INDEX adu5sat_now_index ON adu5_sat (now);

-- G12 GPS position and velocity table
CREATE TABLE g12_pos (
	nbuf int PRIMARY KEY,
	crc int2,   -- See comment on top   
	now int,   -- time of processing
	time int, 
	us int,
	tod int,      -- Time of Day in ms
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
	unit int2,  -- G12 unit # 
	UNIQUE(time,us)
) WITHOUT OIDS;
CREATE INDEX g12pos_now_index ON g12_pos (now);

-- G12 GPS satellite info table
CREATE TABLE g12_sat (
	nbuf int PRIMARY KEY,
	crc int2,   -- See comment on top   
	now int,   -- time of processing
	time int UNIQUE, 
	numsats int2,      -- Number of satellites
	prn int2[],        -- PRN value (numsats, up to 20)
	elevation int2[],  -- Satellite elevation (numsats, up to 20)
	snr int2[],        -- Satellite signal SNR (numsats, up to 20)
	flag int2[],       -- Quality flag (numsats, up to 20)
	azimuth int4[]     -- Satellite azimuth (numsats, up to 20)
) WITHOUT OIDS;
CREATE INDEX g12sat_now_index ON g12_sat (now);

-- Command echo table
CREATE TABLE cmd (
	nbuf int PRIMARY KEY,
	crc int2,    -- See comment on top
	now int,   -- time of processing
	time int UNIQUE,
	flag int2 CHECK (flag=0 OR flag=1),
	bytes int2, -- Number of bytes
	cmd int2[]  -- command bytes, up to 10
) WITHOUT OIDS;
CREATE INDEX cmd_now_index ON cmd (now);

-- Wakeup packet table
CREATE TABLE wakeup (
	nbuf int PRIMARY KEY,
	crc int2,    -- See comment on top
	now int,   -- time of processing
	type int2 CHECK (type>=0 AND type<=3)  -- 0=LOS, 1=HIRATE, 2=COMM 1, 3=COMM 2
);

-- Configuration file table
CREATE TABLE file (
	nbuf int PRIMARY KEY,
	crc int2,    -- See comment on top
	now int,   -- time of processing
	time int,  -- time of file dump
	filename text,  -- Name of the file
	length int,     -- Lenght of file in bytes
	content text,    -- File content
	UNIQUE(time,filename)
) WITHOUT OIDS;
CREATE INDEX file_now_index ON file (now);

-- Slow rate packets
CREATE TABLE slow (
	nbuf int, -- disable for now PRIMARY KEY,
	crc int2,    -- See comment on top
	now int,   -- time of processing
	time int UNIQUE, 
	evnum int,       -- last event number
	latitude real,   -- last payload latitude
	longitude real,  -- last payload longitude
	altitude real,   -- last payload altitude
	rate1 real,  -- 1 minute event rate 
	rate10 real, -- 10 minute event rate
	tempraw int2[], -- Housekeeping temperatures, 8 entries, ADC counts (except tempraw[1] which is in degrees)
	powerraw int2[], -- Housekeeping voltages and currents, 4 entries, ADC counts
	tempv real[], -- Housekeeping temperatures, 8 entries, voltages (except tempraw[1] which is 0)
	powerv real[], -- Housekeeping voltages and currents, 4 entries, voltages
	temp real[], -- Housekeeping temperatures, 8 entries, degrees C
	ppvv real,  -- PV voltage
	p24v real,  -- +24V voltage
	bati real,  -- Battery current
	p24i real,  -- +24V current
--	avgl1 int2[], -- Average L1 trigger rates per trigger surf, 8 entries (divided by 512)
--	avgl2 int2[][], -- Average level 2 rate (2 layers, 16 sectors, divided by 64)
--	avgl2 int2[], -- Average level 2 rate (2 layers, 16 sectors, divided by 64), average of 2 layers, now.
	avgl3 int2[],   -- Average level 3 rate (16 sectors, multiplied by 32)
	avgscaler int2[][], -- Average scaler rates (8 surfs by 4 antennas) in kHz(?)
--	rmsscaler int2[][], -- RMS of scaler rates (8 surfs by 4 antennas)
	avgrfpow int2[][] -- Average RF power ADC values, (9 surfs by 8 RF channels)
) WITHOUT OIDS;
CREATE INDEX slow_now_index ON slow (now);

-- Create function which will calculate voltages for HK Acromag entries
-- and calibrate physical values for slow packet data
CREATE OR REPLACE FUNCTION calslow() RETURNS trigger AS '
  DECLARE 
    caltime int;
    avztime int;
    hkcal RECORD;
    hkavz RECORD;
    cal_c int;
    avz_c int;
  BEGIN
  -- Find last CAL packet
     SELECT INTO caltime MAX(time) FROM hk_cal WHERE (code=768 AND (crc=257 OR crc=1) AND (time<=NEW.time));
     IF NOT FOUND THEN
    	RAISE WARNING ''No CAL hk packet before time %d'', NEW.time;
	RETURN NEW;
     ELSE 
        SELECT INTO hkcal nbuf,bd1,bd2,bd3 FROM hk_cal WHERE time=caltime AND code=768;
     END IF;

  -- Find last AVZ packet
     SELECT INTO avztime MAX(time) FROM hk_cal WHERE (code=512 AND (crc=257 OR crc=1) AND (time<=NEW.time));
     IF NOT FOUND THEN
    	RAISE WARNING ''No AVZ hk packet before time %d'', NEW.time;
	RETURN NEW;
     ELSE 
        SELECT INTO hkavz nbuf,bd1,bd2,bd3 FROM hk_cal WHERE time=avztime AND code=512;
     END IF;

     NEW.tempv:=''{}'';
     NEW.powerv:=''{}'';
     NEW.temp:=''{}'';	
     cal_c=4055;    --constant
     avz_c=2048;    --constant

-- Calibrate voltages
     NEW.tempv[1] := 0;
     NEW.tempv[2] := 4.9 / (cal_c-avz_c) * (NEW.tempraw[2] - avz_c);
     NEW.tempv[3] := 4.9 / (cal_c-avz_c) * (NEW.tempraw[3] - avz_c);
     NEW.tempv[4] := 4.9 / (cal_c-avz_c) * (NEW.tempraw[4] - avz_c);
     NEW.tempv[5] := 4.9 / (cal_c-avz_c) * (NEW.tempraw[5] - avz_c);
     NEW.tempv[6] := 4.9 / (cal_c-avz_c) * (NEW.tempraw[6] - avz_c);
     NEW.tempv[7] := 4.9 / (cal_c-avz_c) * (NEW.tempraw[7] - avz_c);
     NEW.tempv[8] := 4.9 / (cal_c-avz_c) * (NEW.tempraw[8] - avz_c);

     NEW.powerv[1] := 4.9 / (cal_c-avz_c) * (NEW.powerraw[1] - avz_c);
     NEW.powerv[2] := 4.9 / (cal_c-avz_c) * (NEW.powerraw[2] - avz_c);
     NEW.powerv[3] := 4.9 / (cal_c-avz_c) * (NEW.powerraw[3] - avz_c);
     NEW.powerv[4] := 4.9 / (cal_c-avz_c) * (NEW.powerraw[4] - avz_c);

-- Create physical values based on documentation
     NEW.temp[1] := NEW.tempraw[1];
     FOR i IN 2..8 LOOP
        NEW.temp[i] := NEW.tempv[i]*100-273.15;
     END LOOP;

     NEW.ppvv:= NEW.powerv[1]*18.252;
     NEW.p24v:= NEW.powerv[2]*10.1377;
     NEW.bati:= NEW.powerv[3]*20;
     NEW.p24i:= NEW.powerv[4]*20;

     RETURN NEW;
  END;
' LANGUAGE plpgsql;
CREATE TRIGGER calslow BEFORE INSERT OR UPDATE ON slow
    FOR EACH ROW EXECUTE PROCEDURE calslow();

-- Header table
CREATE TABLE hd (
	nbuf int PRIMARY KEY,
	crc int2,   -- See comment on top   
	now int,   -- time of processing
	time int,
	us   int,  -- UNIX microsecond
        ns   int,  -- Nanosecond part of second from GPS
	evid int,  -- turf event ID ; 12bit(run#) + 20bit(ev# in run)
	evnum int UNIQUE, -- Event number
	surfmask int2, -- SURF mask (12-bit value)
	calib int, -- Calibration status 
	priority int2,  -- Event priority 
	turfword int2, -- Turf upper word
	l1mask int, -- l1 trigger mask--peng
	l1maskh int, -- l1 trigger maskH--peng
	phimask int, -- phi trigger mask
	phimaskh int, -- phi trigger mask--peng
        peakthetabin int2, -- 8-bit peak theta bin from Prioritizer
        imagepeak int, -- 16-bit image peak from Prioritizer
        coherentsumpeak int, -- 16-bit coherent sum peak from Prioritizer
        prioritizerstuff int, -- TBD

	--otherphimask int, --other phi trigger mask
	--otherphimaskh int, --other phi trigger mask--peng
	trigtype int2, -- TURFIO trigger type
        trignum int,   -- TURFIO trig number
        l3cnt   int2,  -- TURFIO L3 type1 count
        pps int,    -- TURFIO pps number
	trigtime int,  -- TURFIO trig time
	c3po int,   -- TURFIO C3PO number
	deadtime int2, -- TURFIO deadtime
	l3trigpat int,   -- l3TrigPatternH;
	l3trigpath int,   -- l3TrigPatternH;--peng
--	wvin int2,       -- Track how many waveforms are present
	UNIQUE (time,us)
) WITHOUT OIDS;
CREATE INDEX hd_now_index ON hd (now);
-- CREATE VIEW rf AS SELECT * FROM hd WHERE wvin>0;

-- Raw waveform table
CREATE TABLE wv (
	nbuf int,  -- Don't key nbuf since multiple waveforms can come in one buffer
	crc int2,   -- See comment on top
	now int,   -- time of processing
	evnum int, -- Reference to header 
	id int2 CHECK (id>=0 AND id<=107),    -- Channel id number 9*12=108
	chip int2 CHECK (chip>=0 AND chip<=3),  -- Chip read out
	rcobit int2 CHECK (rcobit=0 OR rcobit=1), -- RCObit
	hbwrap int2 CHECK (hbwrap=0 OR hbwrap=1), -- Hit bus wrapped
	hbstart int2 CHECK (hbstart>=0 AND hbstart<260),  -- Position of first hitbus bin before rotation
        hbend int2 CHECK(hbend>=0 AND hbend<260),    -- Position of last hitbus bin before rotation
	peds int4,    -- Timestamp of peds used
	raw int2[],    -- Raw data, full 16 bits (post decoding if necessary), 260 values
	cal real[],     -- Calibrated and rotated data (mV), 260 values
	UNIQUE (evnum,id)
) WITHOUT OIDS;
CREATE VIEW rf AS SELECT * FROM hd WHERE 
     (SELECT count(evnum) FROM wv WHERE evnum=hd.evnum LIMIT 1)>0;


-- Fixed table with pedestal values for waveforms
CREATE TABLE wv_ped (
	nbuf int,   -- Don't key nbuf since multiple pedestals can come in one buffer
	time int,   -- Time reference of pedestals (actually pedestal taking end time)
	crc int2,   -- See comment on top
	now int,    -- time of processing
	start int,  -- Pedestal taking start time
	id int2 CHECK (id>=0 AND id<=107),     -- Channel id number 9*12=108
	chip int2 CHECK (chip>=0 AND chip<=3),   -- Chip (0-3)
	entries int2, -- Number of samples taken to calculate pedestals
	ped real[],   -- Pedestal values, 260 per chip (divided by 2)
	rms real[]    -- Pedestal RMS values, 260 per chip
) WITHOUT OIDS;
CREATE INDEX wv_ped_index ON wv_ped (time);
-- Populate pedestal table; no need any more since data will arrive pedestal subtracted
--COPY wv_ped (time,now,id,chip,ped) FROM '/tmp/pedValues.txt' WITH DELIMITER AS ' ';
\copy wv_ped (time,now,id,chip,ped) FROM '/tmp/pedValues.txt' WITH DELIMITER as ' '
--\copy wv_ped from '/tmp/pedValues.txt' with delimiter ' '

-- Create function which will calibrate waveform entries
CREATE OR REPLACE FUNCTION calwv() RETURNS trigger AS '
  DECLARE 
--     nwv int2;	
     calwv real[]:=''{}'';	-- Calibrated voltage
     rotwv real[]:=''{}'';	-- Rotated waveform
     pedtime int;  
     hdtime int;	
     pedwv real[]:=''{}''; 
     ir int2:=1; -- Index for rotated waveform 
  BEGIN
-- If this is UPDATE and no change in raw data, do nothing
     IF TG_OP = ''UPDATE'' THEN
       IF NEW.raw = OLD.raw THEN
         RETURN NEW;
       END IF;
     END IF;

-- First find corresponding pedestals if needed; 
     IF NEW.peds=0 THEN	
       SELECT INTO hdtime time FROM hd WHERE evnum=NEW.evnum;
       IF NOT FOUND THEN 
          RAISE NOTICE ''No header for event # %'', NEW.evnum;
       -- These will be calibrated later when header arrives; 
          RETURN NEW;
       END IF;
     END IF;

--     SELECT INTO nwv wvin FROM hd WHERE evnum=NEW.evnum;
--     IF NOT FOUND THEN
--	-- Do nothing
--     ELSE
--        nwv:=nwv+1;
--        UPDATE hd SET wvin=nwv WHERE evnum=NEW.evnum;
--     END IF;     

     IF NEW.peds=0 THEN
       SELECT INTO pedtime MAX(time) FROM wv_ped WHERE time<=hdtime AND (crc=257 OR crc=1);
       IF NOT FOUND THEN 
  	  RAISE WARNING ''No pedestals before time %'',hdtime; 
          RETURN NEW;
       END IF;
       SELECT INTO pedwv ped FROM wv_ped WHERE time=pedtime AND id=NEW.id AND chip=NEW.chip;
       IF pedwv IS NULL THEN
         RAISE WARNING ''No pedestal record for time=% id=% chip=%'',pedtime,NEW.id,NEW.chip;
 	 RETURN NEW;
       END IF;
     END IF;

-- Calibrate voltages
     FOR i IN 1..260 LOOP
	-- First handle zeros in first bin
          IF NEW.raw[i]=0 THEN
            calwv[i]:=0;
          ELSE  
            IF NEW.peds=0 THEN -- Subtract pedestal values and gain correction; 
	      calwv[i] := (NEW.raw[i]-pedwv[i]*2)*1.17;  -- mV/ADC, per Gary email of 05/04/2006; pedestals x2 since they are bit shifted 
	    ELSE 
 	      calwv[i] := NEW.raw[i]*2*1.17; -- x2 for bit shift, x1.17 mv/ADC, per Gary email of 05/04/2006 
            END IF;
	  END IF; 
     END LOOP;
     
     -- Record which peds were used
     NEW.peds=pedtime;		

-- Perform rotation 
    IF NEW.hbwrap=1 THEN -- Wrapped hitbus
	FOR i IN (NEW.hbstart+2)..NEW.hbend LOOP
	   rotwv[ir]:=calwv[i];	
	   ir:=ir+1;
	END LOOP;
    ELSE
	FOR i IN (NEW.hbend+2)..260 LOOP  
	   rotwv[ir]:=calwv[i];	
	   ir:=ir+1;
	END LOOP;
        FOR i IN 2..NEW.hbstart LOOP  -- Ignore SCA=0
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
--amirs blocked code on august 25 2008------
--CREATE TRIGGER calwv BEFORE INSERT OR UPDATE ON wv
--    FOR EACH ROW EXECUTE PROCEDURE calwv();

-- Create function which will calibrate waveform entries
CREATE OR REPLACE FUNCTION calwv1() RETURNS trigger AS '
  DECLARE 
--     nwv int2;	
     calwv real[]:=''{}'';	-- Calibrated voltage
     rotwv real[]:=''{}'';	-- Rotated waveform
     pedtime int;  
     hdtime int;	
     pedwv real[]:=''{}''; 
     ir int2:=1; -- Index for rotated waveform 
  BEGIN
-- If this is UPDATE and no change in raw data, do nothing
     IF TG_OP = ''UPDATE'' THEN
       IF NEW.raw = OLD.raw THEN
         RETURN NEW;
       END IF;
     END IF;

-- First find corresponding pedestals if needed; 
     IF NEW.peds=0 THEN	
       SELECT INTO hdtime time FROM hd WHERE evnum=NEW.evnum;
       IF NOT FOUND THEN 
          RAISE NOTICE ''No header for event # %'', NEW.evnum;
       -- These will be calibrated later when header arrives; 
          RETURN NEW;
       END IF;
     END IF;

--     SELECT INTO nwv wvin FROM hd WHERE evnum=NEW.evnum;
--     IF NOT FOUND THEN
--	-- Do nothing
--     ELSE
--        nwv:=nwv+1;
--        UPDATE hd SET wvin=nwv WHERE evnum=NEW.evnum;
--     END IF;     

     IF NEW.peds=0 THEN
       SELECT INTO pedtime MAX(time) FROM wv_ped WHERE time<=hdtime AND (crc=257 OR crc=1);
       IF NOT FOUND THEN 
  	  RAISE WARNING ''No pedestals before time %'',hdtime; 
          RETURN NEW;
       END IF;
       SELECT INTO pedwv ped FROM wv_ped WHERE time=pedtime AND id=NEW.id AND chip=NEW.chip;
       IF pedwv IS NULL THEN
         RAISE WARNING ''No pedestal record for time=% id=% chip=%'',pedtime,NEW.id,NEW.chip;
 	 RETURN NEW;
       END IF;
     END IF;

-- Calibrate voltages
     FOR i IN 1..260 LOOP
	-- First handle zeros in first bin
          IF NEW.raw[i]=0 THEN
            calwv[i]:=0;
	    ELSE 
 	      calwv[i] := NEW.raw[i]*2*1.17; -- x2 for bit shift, x1.17 mv/ADC, per Gary email of 05/04/2006 
	  END IF; 
     END LOOP;
     
     -- Record which peds were used
     NEW.peds=pedtime;		

-- Perform rotation 
    IF NEW.hbwrap=1 THEN -- Wrapped hitbus
	FOR i IN (NEW.hbstart+2)..NEW.hbend LOOP
	   rotwv[ir]:=calwv[i];	
	   ir:=ir+1;
	END LOOP;
    ELSE
	FOR i IN (NEW.hbend+2)..260 LOOP  
	   rotwv[ir]:=calwv[i];	
	   ir:=ir+1;
	END LOOP;
        FOR i IN 2..NEW.hbstart LOOP  -- Ignore SCA=0
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
--amirs blocked code on august 26 2008
--CREATE TRIGGER calwv1 BEFORE INSERT OR UPDATE ON wv
--    FOR EACH ROW EXECUTE PROCEDURE calwv1();
-- Create function which will retroactively calibrate waveforms which arrive
-- before header
CREATE OR REPLACE FUNCTION retrocalwv(int2,int,int) RETURNS void AS '
  DECLARE 
     calwv real[]:=''{}'';	-- Calibrated voltage
     rotwv real[]:=''{}'';	-- Rotated waveform
     pedtime int;
     wvid ALIAS FOR $1;
     wvevnum ALIAS FOR $2;	
     hdtime ALIAS FOR $3;	
     pedwv real[]:=''{}'';
     wvrec RECORD;
     ir int2:=1; -- Index for rotated waveform 
  BEGIN
-- First find corresponding pedestals	
     SELECT INTO pedtime MAX(time) FROM wv_ped WHERE time<=hdtime;
--amirs addition for august21 2008
--RAISE WARNING ''We are in retrocalwv for hdtime %'',hdtime; 
     IF NOT FOUND THEN 
 	RAISE WARNING ''No pedestals before time %'',hdtime; 
        RETURN;
     END IF;

     SELECT INTO wvrec * FROM wv WHERE id=wvid AND evnum=wvevnum;
     SELECT INTO pedwv ped FROM wv_ped WHERE time=pedtime AND id=wvid AND chip=wvrec.chip;
     IF NOT FOUND THEN 
        RAISE WARNING ''No pedestal record for time=% id=% chip=%'',pedtime,wvid,wvrec.chip;
 	RETURN;
     END IF;

-- Calibrate voltages
     FOR i IN 1..260 LOOP
	-- First handle zeros in first bin
        IF wvrec.raw[i]=0 THEN
          calwv[i]:=0;
        ELSE  
  	  -- Subtract pedestal values and linearity correction
	  calwv[i] := (wvrec.raw[i]-pedwv[i]*2)*1.17;  -- mV/ADC, per Gary email of 05/04/2006; x2 for pedestals since they are bit shifted
        END IF;
     END LOOP;
     
-- Perform rotation 
     IF wvrec.hbwrap=1 THEN -- Wrapped hitbus
	FOR i IN (wvrec.hbstart+2)..wvrec.hbend LOOP
	   rotwv[ir]:=calwv[i];	
	   ir:=ir+1;
	END LOOP;
     ELSE
	FOR i IN (wvrec.hbend+2)..260 LOOP  
	   rotwv[ir]:=calwv[i];	
	   ir:=ir+1;
	END LOOP;
        FOR i IN 2..wvrec.hbstart LOOP  -- Ignore first SCA=0
	   rotwv[ir]:=calwv[i];	
	   ir:=ir+1;
	END LOOP;
     END IF;
     FOR i in ir..260 LOOP  -- Fill in remaining bins with zeros
       rotwv[i]:=0;
     END LOOP;

     UPDATE wv SET cal=rotwv WHERE id=wvid AND evnum=wvevnum;	
     UPDATE wv SET peds=pedtime WHERE id=wvid AND evnum=wvevnum;
	
     RETURN;
  END;
' LANGUAGE plpgsql;

-- Trigger function to be run when new event header is inserted
CREATE OR REPLACE FUNCTION newhd() RETURNS trigger AS '
  DECLARE 
--     wvin int2:=0;	
     wvrec RECORD;		
  BEGIN
--amirs addition for august21 2008
--RAISE WARNING ''We are in newhd for evnum %'',NEW.evnum;
-- Now check if any waveforms already arrived 
     FOR wvrec IN SELECT id,peds FROM wv WHERE evnum=NEW.evnum LOOP
--        wvin:=wvin+1;
	IF wvrec.peds=0 THEN
	   -- Now need to calibrate event retroactively
  	   PERFORM retrocalwv(wvrec.id,NEW.evnum,NEW.time);
	END IF;
     END LOOP;
--     NEW.wvin:=wvin;

     RETURN NEW;
  END;
' LANGUAGE plpgsql;
--amirs code blocked on august 26 2008
--CREATE TRIGGER newhd BEFORE INSERT ON hd
--    FOR EACH ROW EXECUTE PROCEDURE newhd();

-- Function that calculates current event rate based on latest event headers
CREATE OR REPLACE FUNCTION eventrate(int4) RETURNS real AS '
DECLARE
    dt ALIAS for $1;
    hdrec1 RECORD;
    hdrec2 RECORD;
    rate real;
BEGIN
    SELECT INTO hdrec1 time,evnum FROM hd WHERE crc=257 ORDER BY time DESC,us DESC  LIMIT 1;
    IF NOT FOUND THEN
      RETURN -1;
    END IF;	
    SELECT INTO hdrec2 time,evnum FROM hd WHERE crc=257 AND time<hdrec1.time-dt ORDER BY time DESC, us DESC LIMIT 1;
    IF NOT FOUND THEN
      RETURN -1;
    END IF;
    rate:=(hdrec1.evnum-hdrec2.evnum)*1.0/(hdrec1.time-hdrec2.time);
    RETURN rate;	
END;
' LANGUAGE plpgsql;

-- Run start packets
CREATE TABLE run (
	nbuf int PRIMARY KEY, 
	crc int2,    -- See comment on top
	now int,   -- time of processing
	time int UNIQUE, 
	evnum int,       -- start event number
	run int          -- run number
) WITHOUT OIDS;
CREATE INDEX run_evnum_index ON run (evnum);

-- Other monitor packets
CREATE TABLE other (
	nbuf int PRIMARY KEY, 
	crc int2,    -- See comment on top
	now int,     -- time of processing
	time int UNIQUE, 
        raminodes int,  -- RAM disk inodes
    	runtime int,    -- Run start time
        runevnum int,   -- Run start event number
        runnum int,     -- Run number
	filesacqd int,    -- Files in /tmp/anita/acqd
        fileseventd int,  -- Files in /tmp/anita/eventd
	filesprior int,   -- Files in /tmp/anita/prioritizerd    
	lnacqd int,    -- Links in /tmp/anita/acqd
        lneventd int,  -- Links in /tmp/anita/eventd
	lnprior int,   -- Links in /tmp/anita/prioritizerd    
	other int      -- Other flag
) WITHOUT OIDS;
CREATE INDEX other_runevnum_index ON other (runevnum);

-- ISSUE NOTIFICATIONS FOR ALL TABLES HERE --
CREATE RULE hk_new AS ON INSERT TO hk DO NOTIFY hk;
CREATE RULE SShk_new AS ON INSERT TO SShk DO NOTIFY SShk;
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
CREATE RULE slow_new AS ON INSERT TO slow DO NOTIFY slow;
CREATE RULE file_new AS ON INSERT TO file DO NOTIFY file;
CREATE RULE rf_new1 AS ON INSERT TO hd DO NOTIFY rf;
CREATE RULE rf_new2 AS ON UPDATE TO hd DO NOTIFY rf;

-- Create user apache and allow read access to all tables for web access
--CREATE USER apache;
--GRANT SELECT ON hk TO apache;
--GRANT SELECT ON hk_cal TO apache;
--GRANT SELECT ON sshk_cal TO apache;
--GRANT SELECT ON hk_surf TO apache;
--GRANT SELECT ON turf TO apache;
--GRANT SELECT ON mon TO apache;
--GRANT SELECT ON adu5_pat TO apache;
--GRANT SELECT ON adu5_sat TO apache;
--GRANT SELECT ON adu5_vtg TO apache;
--GRANT SELECT ON g12_pos TO apache;
--GRANT SELECT ON g12_sat TO apache;
--GRANT SELECT ON wv TO apache;
--GRANT SELECT ON wv_ped TO apache;
--GRANT SELECT ON hd TO apache;
--GRANT SELECT ON rf TO apache;
--GRANT SELECT ON cmd TO apache;
--GRANT SELECT ON slow TO apache;
--GRANT SELECT ON hk TO apache;
--GRANT SELECT ON file TO apache;

-- Create user gui and allow read access to all tables for viewer access
-- CREATE USER gui;
GRANT SELECT ON hk TO gui;
GRANT SELECT ON SShk TO gui;
GRANT SELECT ON hk_cal TO gui;
GRANT SELECT ON sshk_cal TO gui;
GRANT SELECT ON hk_surf TO gui;
GRANT SELECT ON turf TO gui;
GRANT SELECT ON rtlsdr TO gui;
GRANT SELECT ON tuff_status TO gui;
GRANT SELECT ON tuff_cmd TO gui;
GRANT SELECT ON mon TO gui;
GRANT SELECT ON adu5_pat TO gui;
GRANT SELECT ON adu5_sat TO gui;
GRANT SELECT ON adu5_vtg TO gui;
GRANT SELECT ON g12_pos TO gui;
GRANT SELECT ON g12_sat TO gui;
GRANT SELECT ON wv TO gui;
GRANT SELECT ON wv_ped TO gui;
GRANT SELECT ON hd TO gui;
GRANT SELECT ON rf TO gui;
GRANT SELECT ON cmd TO gui;
GRANT SELECT ON slow TO gui;
GRANT SELECT ON hk TO gui;
GRANT SELECT ON file TO gui;
GRANT SELECT ON run TO gui;
GRANT SELECT ON other TO gui;
