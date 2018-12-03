import requests
import pprint
def cloth_judge():
    cloth_best_temperature = [7, 20, 25, 15, 35]  #[コート, ジャケット, シャツ, ニット, Tシャツ]
    send_data = [0,0,0,0,0]
    max_temprature_default = '20'
    min_temprature_default = '10'
    url = 'http://weather.livedoor.com/forecast/webservice/json/v1?city=130010'
    api_data = requests.get(url).json()
    print(api_data)
    for weather in api_data['forecasts']:
        weather_date = weather['dateLabel']
        # if(weather_date == '今日') :
        if(weather_date == '明日') :
            weather_temprature = weather['temperature']
            weather_max_temp = weather_temprature['max']
            if(isinstance(weather_max_temp,type(None))):
                weather_max_celsius = max_temprature_default;
            else:
                weather_max_celsius = weather_max_temp['celsius']
            print('max_temprature:'+weather_max_celsius)
            weather_min_temp = weather_temprature['min']
            if(isinstance(weather_min_temp,type(None))):
                weather_min_celsius = min_temprature_default;
            else:
                weather_min_celsius = weather_min_temp['celsius']
            print('min_temprature:'+weather_min_celsius)
            weather_forecasts = weather['telop']
            print(weather_date + ':' + weather_forecasts)
            temperature = (float(weather_max_celsius) + float(weather_min_celsius))/2
            print('平均気温:'+str(temperature))
            # temperature = 20
            for i in range(4):
                cloth_val = abs(cloth_best_temperature[i] - temperature)
                if(cloth_val<=5):
                    send_data[i] = 1
                elif(temperature<=15):
                    send_data[i] = 2
                elif(20<temperature<=30):
                    send_data[i] = 3
                else:
                    send_data[i] = 0
            return send_data


if __name__ == '__main__' :
    print(cloth_judge())
