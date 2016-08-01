-- Trigger function to be run when new hk data is inserted
CREATE OR REPLACE FUNCTION newhk() RETURNS trigger AS '
  DECLARE 
     dt integer := 60;
     hdrec RECORD;
     hktime int;
     hkus int;
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
	 SELECT INTO hktime,hkus time,us FROM hk WHERE nbuf=hdrec.hk;
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
     pattime int;
     patus int;	
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
     surftime int;
     surfus int;	
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
     turftime int;
     turfus int;
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
     cmdtime int;
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
--CREATE TRIGGER newhk AFTER INSERT OR UPDATE ON hk
--    FOR EACH ROW EXECUTE PROCEDURE newhk();
--CREATE TRIGGER newpat AFTER INSERT OR UPDATE ON adu5_pat
--    FOR EACH ROW EXECUTE PROCEDURE newpat();
--CREATE TRIGGER newsurf AFTER INSERT OR UPDATE ON hk_surf
--    FOR EACH ROW EXECUTE PROCEDURE newsurf();
--CREATE TRIGGER newturf AFTER INSERT OR UPDATE ON turf
--    FOR EACH ROW EXECUTE PROCEDURE newturf();
--CREATE TRIGGER newcmd AFTER INSERT OR UPDATE ON cmd
--    FOR EACH ROW EXECUTE PROCEDURE newcmd();

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

-- Function that finds closest time match in given table to given time within specified time span
CREATE OR REPLACE FUNCTION findhk(int,int,int) RETURNS int AS '
  DECLARE
     t_in ALIAS FOR $1;
     us_in ALIAS FOR $2;
     dt ALIAS FOR $3;
     refpt real;
     refnt real;
     refnbuf int;
  BEGIN
     SELECT INTO refnt MAX(us_in/1e6-us/1e6) FROM hk WHERE (time=t_in AND us<=us_in);
     IF NOT FOUND THEN
     	  SELECT INTO refnt MIN((t_in+us_in/1e6)-(time+us/1e6)) FROM hk WHERE (time>(t_in-dt) AND time<t_in);
          IF NOT FOUND THEN
	      refnt:=-1;
          END IF;
     END IF;

     SELECT INTO refpt MIN(us/1e6-us_in/1e6) FROM hk WHERE (time=t_in AND us>us_in);
     IF NOT FOUND THEN
          SELECT INTO refpt MIN((time+us/1e6)-(t_in+us_in/1e6)) FROM hk WHERE (time>t_in AND time<(t_in+dt));
          IF NOT FOUND THEN
              refpt:=-1;
          END IF;
     END IF;

     IF refnt>=0 AND refpt>=0 THEN -- Events found on both sides
       IF refnt<refpt THEN
	 SELECT INTO refnbuf nbuf FROM hk WHERE (time+us/1e6)=(t_in+us_in/1e6)-refnt LIMIT 1;
       ELSE
	 SELECT INTO refnbuf nbuf FROM hk WHERE (time+us/1e6)=(t_in+us_in/1e6)+refpt LIMIT 1;
       END IF;
     ELSIF refnt>=0 THEN  -- Only preceeding events found
	SELECT INTO refnbuf nbuf FROM hk WHERE (time+us/1e6)=(t_in+us_in/1e6)-refnt LIMIT 1;
     ELSIF refpt>=0 THEN  -- Only trailing events found
	SELECT INTO refnbuf nbuf FROM hk WHERE (time+us/1e6)=(t_in+us_in/1e6)+refpt LIMIT 1;
     END IF;

     RETURN refnbuf;
  END;
' LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION findsurf(int,int,int) RETURNS int AS '
  DECLARE
     t_in ALIAS FOR $1;
     us_in ALIAS FOR $2;
     dt ALIAS FOR $3;
     refpt real;
     refnt real;
     refnbuf int;
  BEGIN
     SELECT INTO refnt MAX(us_in/1e6-us/1e6) FROM hk_surf WHERE (time=t_in AND us<=us_in);
     IF NOT FOUND THEN
     	  SELECT INTO refnt MIN((t_in+us_in/1e6)-(time+us/1e6)) FROM hk_surf WHERE (time>(t_in-dt) AND time<t_in);
          IF NOT FOUND THEN
	      refnt:=-1;
          END IF;
     END IF;

     SELECT INTO refpt MIN(us/1e6-us_in/1e6) FROM hk_surf WHERE (time=t_in AND us>us_in);
     IF NOT FOUND THEN
          SELECT INTO refpt MIN((time+us/1e6)-(t_in+us_in/1e6)) FROM hk_surf WHERE (time>t_in AND time<(t_in+dt));
          IF NOT FOUND THEN
              refpt:=-1;
          END IF;
     END IF;

     IF refnt>=0 AND refpt>=0 THEN -- Events found on both sides
       IF refnt<refpt THEN
	 SELECT INTO refnbuf nbuf FROM hk_surf WHERE (time+us/1e6)=(t_in+us_in/1e6)-refnt LIMIT 1;
       ELSE
	 SELECT INTO refnbuf nbuf FROM hk_surf WHERE (time+us/1e6)=(t_in+us_in/1e6)+refpt LIMIT 1;
       END IF;
     ELSIF refnt>=0 THEN  -- Only preceeding events found
	SELECT INTO refnbuf nbuf FROM hk_surf WHERE (time+us/1e6)=(t_in+us_in/1e6)-refnt LIMIT 1;
     ELSIF refpt>=0 THEN  -- Only trailing events found
	SELECT INTO refnbuf nbuf FROM hk_surf WHERE (time+us/1e6)=(t_in+us_in/1e6)+refpt LIMIT 1;
     END IF;

     RETURN refnbuf;
  END;
' LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION findturf(int,int,int) RETURNS int AS '
  DECLARE
     t_in ALIAS FOR $1;
     us_in ALIAS FOR $2;
     dt ALIAS FOR $3;
     refpt real;
     refnt real;
     refnbuf int;
  BEGIN
     SELECT INTO refnt MAX(us_in/1e6-us/1e6) FROM turf WHERE (time=t_in AND us<=us_in);
     IF NOT FOUND THEN
     	  SELECT INTO refnt MIN((t_in+us_in/1e6)-(time+us/1e6)) FROM turf WHERE (time>(t_in-dt) AND time<t_in);
          IF NOT FOUND THEN
	      refnt:=-1;
          END IF;
     END IF;

     SELECT INTO refpt MIN(us/1e6-us_in/1e6) FROM turf WHERE (time=t_in AND us>us_in);
     IF NOT FOUND THEN
          SELECT INTO refpt MIN((time+us/1e6)-(t_in+us_in/1e6)) FROM turf WHERE (time>t_in AND time<(t_in+dt));
          IF NOT FOUND THEN
              refpt:=-1;
          END IF;
     END IF;

     IF refnt>=0 AND refpt>=0 THEN -- Events found on both sides
       IF refnt<refpt THEN
	 SELECT INTO refnbuf nbuf FROM turf WHERE (time+us/1e6)=(t_in+us_in/1e6)-refnt LIMIT 1;
       ELSE
	 SELECT INTO refnbuf nbuf FROM turf WHERE (time+us/1e6)=(t_in+us_in/1e6)+refpt LIMIT 1;
       END IF;
     ELSIF refnt>=0 THEN  -- Only preceeding events found
	SELECT INTO refnbuf nbuf FROM turf WHERE (time+us/1e6)=(t_in+us_in/1e6)-refnt LIMIT 1;
     ELSIF refpt>=0 THEN  -- Only trailing events found
	SELECT INTO refnbuf nbuf FROM turf WHERE (time+us/1e6)=(t_in+us_in/1e6)+refpt LIMIT 1;
     END IF;

     RETURN refnbuf;
  END;
' LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION findgps(int,int,int) RETURNS int AS '
  DECLARE
     t_in ALIAS FOR $1;
     us_in ALIAS FOR $2;
     dt ALIAS FOR $3;
     refpt real;
     refnt real;
     refnbuf int;
  BEGIN
     SELECT INTO refnt MAX(us_in/1e6-us/1e6) FROM adu5_pat WHERE (time=t_in AND us<=us_in);
     IF NOT FOUND THEN
     	  SELECT INTO refnt MIN((t_in+us_in/1e6)-(time+us/1e6)) FROM adu5_pat WHERE (time>(t_in-dt) AND time<t_in);
          IF NOT FOUND THEN
	      refnt:=-1;
          END IF;
     END IF;

     SELECT INTO refpt MIN(us/1e6-us_in/1e6) FROM adu5_pat WHERE (time=t_in AND us>us_in);
     IF NOT FOUND THEN
          SELECT INTO refpt MIN((time+us/1e6)-(t_in+us_in/1e6)) FROM adu5_pat WHERE (time>t_in AND time<(t_in+dt));
          IF NOT FOUND THEN
              refpt:=-1;
          END IF;
     END IF;

     IF refnt>=0 AND refpt>=0 THEN -- Events found on both sides
       IF refnt<refpt THEN
	 SELECT INTO refnbuf nbuf FROM adu5_pat WHERE (time+us/1e6)=(t_in+us_in/1e6)-refnt LIMIT 1;
       ELSE
	 SELECT INTO refnbuf nbuf FROM adu5_pat WHERE (time+us/1e6)=(t_in+us_in/1e6)+refpt LIMIT 1;
       END IF;
     ELSIF refnt>=0 THEN  -- Only preceeding events found
	SELECT INTO refnbuf nbuf FROM adu5_pat WHERE (time+us/1e6)=(t_in+us_in/1e6)-refnt LIMIT 1;
     ELSIF refpt>=0 THEN  -- Only trailing events found
	SELECT INTO refnbuf nbuf FROM adu5_pat WHERE (time+us/1e6)=(t_in+us_in/1e6)+refpt LIMIT 1;
     END IF;

     RETURN refnbuf;
  END;
' LANGUAGE plpgsql;

-- Funcrion that cross references headers with associated events
CREATE OR REPLACE FUNCTION crossrefhd() RETURNS void AS '
  DECLARE 
     refpt int;
     wvrec RECORD;
     hdrec RECORD;			
     ref int;	
  BEGIN

-- Find nearby hk event
     FOR hdrec IN SELECT nbuf,time,us FROM hd WHERE hk IS NULL LOOP
  	SELECT INTO ref findhk(hdrec.time,hdrec.us,30);
	UPDATE hd SET hk=ref WHERE nbuf=hdrec.nbuf;
     END LOOP;

-- Find nearby SURF hk event
     FOR hdrec IN SELECT nbuf,time,us FROM hd WHERE surf IS NULL LOOP
 	SELECT INTO ref findsurf(hdrec.time,hdrec.us,30);
	UPDATE hd SET surf=ref WHERE nbuf=hdrec.nbuf;
     END LOOP;
	
-- Find nearby TURF rate event
     FOR hdrec IN SELECT nbuf,time,us FROM hd WHERE turf IS NULL LOOP
 	SELECT INTO ref findturf(hdrec.time,hdrec.us,30);
	UPDATE hd SET turf=ref WHERE nbuf=hdrec.nbuf;
     END LOOP;
	
-- Find nearby GPS position event
     FOR hdrec IN SELECT nbuf,time,us FROM hd WHERE pat IS NULL LOOP
 	SELECT INTO ref findgps(hdrec.time,hdrec.us,60);
	UPDATE hd SET pat=ref WHERE nbuf=hdrec.nbuf;
     END LOOP;	

-- Find last CMD echo 
     FOR hdrec IN SELECT nbuf,time,us FROM hd WHERE cmd IS NULL LOOP	
       SELECT INTO refpt MAX(time) FROM cmd WHERE (time<=hdrec.time);
       IF FOUND THEN
          SELECT INTO ref nbuf FROM cmd WHERE time=refpt;
          UPDATE hd SET cmd=ref WHERE nbuf=hdrec.nbuf;
       END IF;
     END LOOP;	

     RETURN ;
  END;
' LANGUAGE plpgsql;

-- Clean up and re-analyze
VACUUM ANALYZE VERBOSE;
-- Cross reference all event headers (this is painfully slow)
SELECT crossrefhd();
