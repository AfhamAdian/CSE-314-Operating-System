class Operative {
    public:
        int id;
        int unitId;
        int stationId;
        bool isLeader;
    
    public:
        Operative ( int id, int unitId, int stationId, bool isLeader ) {
            this->id = id;
            this->unitId = unitId;
            this->stationId = stationId;
            this->isLeader = isLeader;
        }
};

