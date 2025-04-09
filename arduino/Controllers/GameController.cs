using Microsoft.AspNetCore.Mvc;
using MQTTnet;
using MQTTnet.Client;
using System.Collections.Concurrent;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;

public class GameState
{
    public ConcurrentQueue<GameStatus> Messages { get; } = new();

    public event Action OnChange;

    public void AddMessage(GameStatus status)
    {
        Messages.Enqueue(status);
        NotifyStateChanged();
    }

    private void NotifyStateChanged() => OnChange?.Invoke();
}

[ApiController]
[Route("api/game")]
public class GameController : ControllerBase
{
    private readonly GameState _gameState;

    public GameController(GameState gameState) => _gameState = gameState;

    [HttpGet("status")]
    public IActionResult GetStatus() => Ok(_gameState.Messages.ToArray());
}

// GameStatus model to match the expected data structure


public class GameStatus
{
    [JsonPropertyName("player")]
    public string Player { get; set; }

    [JsonPropertyName("action")]
    public string Action { get; set; }

    [JsonPropertyName("symbol")]
    public string Symbol { get; set; }

    [JsonPropertyName("row")]
    public int? Row { get; set; }

    [JsonPropertyName("col")]
    public int? Col { get; set; }
}



public class MqttService : BackgroundService
{
    private readonly GameState _gameState;
    private IMqttClient _mqttClient;
    private IConfiguration _configuration;

    public MqttService(GameState gameState, IConfiguration configuration) {
        _gameState = gameState; _configuration = configuration; }


    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        var options = new MqttClientOptionsBuilder()
            .WithTcpServer(_configuration["HiveMQ:Host"], 8883)
            .WithCredentials(_configuration["HiveMQ:Username"], _configuration["HiveMQ:Password"])
            .WithTls()
            .Build();

        _mqttClient = new MqttFactory().CreateMqttClient();

        _mqttClient.ApplicationMessageReceivedAsync += async e =>
        {
            string message = Encoding.UTF8.GetString(e.ApplicationMessage.PayloadSegment);
            Console.WriteLine($"Received MQTT message: {message}");

            // Deserialize the incoming message to GameStatus object
            var status = JsonSerializer.Deserialize<GameStatus>(message);
            if (status != null)
            {
                _gameState.AddMessage(status); // Add to the queue
            }
        };

        _mqttClient.ConnectedAsync += async e =>
        {
            Console.WriteLine("Connected to MQTT broker.");
            await _mqttClient.SubscribeAsync("game/status");
        };

        _mqttClient.DisconnectedAsync += async e =>
        {
            Console.WriteLine("Disconnected. Trying to reconnect...");
            await Task.Delay(TimeSpan.FromSeconds(5), stoppingToken);
            await _mqttClient.ConnectAsync(options, stoppingToken);
        };

        try
        {
            await _mqttClient.ConnectAsync(options, stoppingToken);
        }
        catch (Exception ex)
        {
            Console.WriteLine($"MQTT Connection failed: {ex.Message}");
        }
    }
}
