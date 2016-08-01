-- Drop existing tables
DROP TABLE hd CASCADE;   -- Event header table 
DROP TABLE wv CASCADE;     -- Waveform table, one channel per entry, raw and calibrated (this is a big table!!!!)
DROP TABLE hk CASCADE;      -- Housekeeping data table
DROP TABLE hk_cal CASCADE;  -- Housekeeping calibration table
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
DROP TABLE slow;    -- Slow control packets
DROP TABLE wakeup;   -- Wake up packets, just log appearances
DROP TABLE file;     -- Configuration file dump table
DROP TABLE run;      -- Run start table
DROP TABLE other;    -- Other monitor table
