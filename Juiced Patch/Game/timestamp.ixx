module;
export module timestamp;
#define Timestamp_ticker *(int*)0x00E843AC

// Simply recreation of the ingame timestamp class, not all functions rn, but it's quite simple!
export class timestamp {
private:
	int value;
public:
	timestamp() {
		value = -1;
	}
    void set(int delta_milliseconds)
    {
        int v2; // eax
        bool v3; // sf
        int v4; // eax

        if (delta_milliseconds < 0)
        {
            v3 = delta_milliseconds + Timestamp_ticker < 0;
            v4 = delta_milliseconds + Timestamp_ticker;
            this->value = delta_milliseconds + Timestamp_ticker;
            if (v3)
                this->value = v4 + 1800000000;
        }
        else
        {
            v2 = delta_milliseconds + Timestamp_ticker;
            this->value = delta_milliseconds + Timestamp_ticker;
            if (v2 > 1800000000)
                this->value = v2 - 1800000000;
        }
    }
    bool elapsed()
    {
        int value; // eax

        value = this->value;
        if (this->value < 0)
            return 0;
        if (value <= Timestamp_ticker)
        {
            if (Timestamp_ticker - value >= 900000000)
                return 0;
        }
        else if (value - Timestamp_ticker < 900000000)
        {
            return 0;
        }
        return 1;
    }
    int time_until()
    {
        if (this->value < 0)
            return 1800000000;
        if (this->value <= Timestamp_ticker)
        {
            if (Timestamp_ticker - this->value <= 900000000)
                return this->value - Timestamp_ticker;
            else
                return this->value + 1800000000 - Timestamp_ticker;
        }
        else if (this->value - Timestamp_ticker <= 900000000)
        {
            return this->value - Timestamp_ticker;
        }
        else
        {
            return -(Timestamp_ticker + 1800000000 - this->value);
        }
    }
    void add_time(int milliseconds) {
        set(time_until() + milliseconds);
    }
    int getvalue() {
        return value;
    }
};