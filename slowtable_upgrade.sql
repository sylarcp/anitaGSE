ALTER TABLE slow ADD tempraw int2[];
ALTER TABLE slow ADD powerraw int2[];
ALTER TABLE slow ADD tempv real[];
ALTER TABLE slow ADD powerv real[];
ALTER TABLE slow DROP temp;
ALTER TABLE slow DROP power;
ALTER TABLE slow ADD temp real[];
ALTER TABLE slow ADD ppvv real;
ALTER TABLE slow ADD p24v real;
ALTER TABLE slow ADD bati real;
ALTER TABLE slow ADD p24i real;

CREATE OR REPLACE FUNCTION calslow() RETURNS trigger AS '
  DECLARE 
    caltime int;
    avztime int;
    hkcal RECORD;
    hkavz RECORD;
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

-- Calibrate voltages
     NEW.tempv[1] := 0;
     NEW.tempv[2] := 4.9 / (hkcal.bd2[1]-hkavz.bd2[1]) * (NEW.tempraw[2] - hkavz.bd2[1]);
     NEW.tempv[3] := 4.9 / (hkcal.bd2[3]-hkavz.bd2[3]) * (NEW.tempraw[3] - hkavz.bd2[3]);
     NEW.tempv[4] := 4.9 / (hkcal.bd2[6]-hkavz.bd2[6]) * (NEW.tempraw[4] - hkavz.bd2[6]);
     NEW.tempv[5] := 4.9 / (hkcal.bd2[37]-hkavz.bd2[37]) * (NEW.tempraw[5] - hkavz.bd2[37]);
     NEW.tempv[6] := 4.9 / (hkcal.bd2[19]-hkavz.bd2[19]) * (NEW.tempraw[6] - hkavz.bd2[19]);
     NEW.tempv[7] := 4.9 / (hkcal.bd2[15]-hkavz.bd2[15]) * (NEW.tempraw[7] - hkavz.bd2[15]);
     NEW.tempv[8] := 4.9 / (hkcal.bd2[36]-hkavz.bd2[36]) * (NEW.tempraw[8] - hkavz.bd2[36]);

     NEW.powerv[1] := 4.9 / (hkcal.bd1[37]-hkavz.bd1[37]) * (NEW.powerraw[1] - hkavz.bd1[37]);
     NEW.powerv[2] := 4.9 / (hkcal.bd1[36]-hkavz.bd1[36]) * (NEW.powerraw[2] - hkavz.bd1[36]);
     NEW.powerv[3] := 4.9 / (hkcal.bd1[15]-hkavz.bd1[15]) * (NEW.powerraw[3] - hkavz.bd1[15]);
     NEW.powerv[4] := 4.9 / (hkcal.bd1[16]-hkavz.bd1[16]) * (NEW.powerraw[4] - hkavz.bd1[16]);

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
