package com.example.wifi;

public class Location {
    private int lat;
    private int lng;

    public Location(int lat, int lng) {
        this.lat = lat;
        this.lng = lng;
    }

    public int getLat(){
        return this.lat;
    }

    public int getLng(){
        return this.lng;
    }


}
